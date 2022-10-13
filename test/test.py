#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Test the given TP shell
#
# Author: David Gonzalez (HEPIA) <david.dg.gonzalez@hesge.ch>

import sys
import tempfile
import subprocess
from pathlib import Path
from colorama import Fore
import logging
import time
import psutil
import re


def print_usage():
    print("Usage: {} shell_executable_name".format(sys.argv[0]))
    print("")
    print("  shell_executable_name: the name of the executable that is produced by the Makefile")
    print("                         if omitted, the script attempt to find it automatically")


# TODO: move in test class (failed so far)
test_failed = False


def test(func):
    def wrapper(self):
        global test_failed
        print(func.__name__ + ": ", end="")
        try:
            func(self)
        except Exception as e:
            print(Fore.RED + "FAILED" + Fore.RESET)
            logging.error(f"{type(e).__name__}:{str(e)}")
            test_failed = True
            return
        print(Fore.GREEN + "SUCCESS" + Fore.RESET)

    return wrapper


class Cmd:
    def __init__(self, cmd) -> None:
        self.cmd = cmd

    def __str__(self) -> str:
        if self.cmd is None:
            return str(None)
        else:
            return ' '.join(self.cmd).strip()

    def __iter__(self) -> str:
        for c in self.cmd:
            yield c


class Shell:
    def __init__(self, executable: str, cwd: str = tempfile.gettempdir()) -> None:
        # Execute the shell and keep track of the process
        self.executable = executable
        self.shell_process = subprocess.Popen(
            [self.executable], cwd=cwd, encoding='utf-8',
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        self.shell_ps = psutil.Process(self.shell_process.pid)

        # Keep track of last executed command
        self.last_cmd = Cmd(None)

        # No ouputs so far...
        self.stdout = None
        self.stderr = None

    # TODO: move out of Shell ?

    def check_ophans(self, cmd: Cmd):
        """ Check if an orphan process (child of 1) was executed with cmd
        cmd: if None the tested command will be the last executed command
        """
        if cmd is None:
            if self.last_cmd is not None:
                cmd = self.last_cmd
            else:
                return
        str_cmd = str(cmd)
        init_ps = psutil.Process(1)
        for p in init_ps.children():
            if p.cmdline() == str_cmd:
                raise AssertionError(
                    'The command "{}" seem to be a child of process 1'.format(str_cmd))

    def wait_children(self, test_zombies: bool = True, timeout: int = 3):
        """ Wait for children of the shell to temrinate
        test_zombies: if True the command will raise an AssertionError if some children are zombies
        timeout: time after which a Timeout exception is raised if there are still children
        """
        # Wait for all children and check zombies / timeout
        start_time = time.time()
        while True:
            # Wait a bit before checking
            time.sleep(0.1)

            # Check if some children are zombies
            children = self.shell_ps.children()
            if test_zombies:
                for c in children:
                    if c.status() == psutil.STATUS_ZOMBIE:
                        raise AssertionError(
                            'The shell child process {} is a zombie (last command executed: {})'.format(c, self.last_cmd))

            # No more children to wait for -> stop looping
            if (len(children) == 0):
                break

            # as the command exceeded the timeout ?
            duration = time.time() - start_time
            if duration > timeout:
                raise psutil.TimeoutExpired(
                    'The process took more than the timeout ({}s) to terminate (last command executed: {})'.format(duration, self.last_cmd))

    def exec_command(self, command: Cmd, wait_cmd: bool = True, timeout: int = 3):
        """Execute a command in the shell without existing the shell
        wait_cmd: wait for all the command processes to finish and raise an error on zombies
        timeout: terminate the command if it last longer than timeout (does not apply if wait_cmd = False)"""
        # Execute the command
        self.shell_process.stdin.write(str(command) + '\n')
        self.shell_process.stdin.flush()
        self.last_cmd = command

        # Wait for the shell childs to finish while checking for zombies
        if wait_cmd:
            self.wait_children(timeout=timeout)

    def exec_commands(self, commands, wait_cmd: bool = True, timeout: int = 3):
        """Execute a list of commands in the shell without existing the shell and exits
        wait_cmd: wait for all the command processes to finish and raise an error on zombies
        timeout: terminate the command if it last longer than timeout (does not apply if wait_cmd = False)"""
        for cmd in commands:
            self.exec_command(cmd, wait_cmd, timeout)
        self.exit()

    def exit(self):
        # We use communicate to be sure that all streams are closed (i.e. process terminated)
        timeout = 1
        try:
            self.stdout, self.stderr = self.shell_process.communicate(
                input='exit\n', timeout=timeout)
        except subprocess.TimeoutExpired:
            self.shell_process.kill()
            raise subprocess.TimeoutExpired(
                'The exit command did not exit the shell after {}s'.format(timeout))
        # TODO: should I check if process is still running at that point (stream closed but process alive ?)

    def read_stdout(self):
        if self.stdout is not None:
            return self.stdout
        else:
            raise ValueError(
                'stdout is None probably because exit was not called on the shell before accessing it')

    def read_stderr(self):
        if self.stderr is not None:
            return self.stderr
        else:
            raise ValueError(
                'stderr is None probably because exit was not called on the shell before accessing it')

    def get_cwd(self):
        return self.shell_ps.cwd()

    def is_running(self):
        return self.shell_process.poll() == None


class Test:
    def __init__(self, shell_exec: str) -> None:
        self.shell_exec = Path(shell_exec).resolve()
        self.shell = None  # currently no shell run
        # TODO: remove self.shell as it should be passed from function to function (or not in case also correct some function that does)

    def _get_exit_code_from_stdout(self, stdout: str) -> int:
        # Find line with keyword "code"
        for line in stdout.splitlines():
            if "exited with code" in line:
                # Assumes that exit code is after keyword code and that it is the only digits
                return int(''.join([c for c in line.split('code')[-1] if c.isdigit()]))
        raise AssertionError('No exit code found')

    def _test_command_results(self, cmd: Cmd, shell: Shell, test_stdout: str, test_stderr: str, test_return: bool):
        """ Test if the results (standard outputs and return code) of a command are the correct ones
        test_stdout/test_stderr: a string indicating if the standard output should include the normal output ('include'),
                                 be empty ('empty'), or not tested ('notest' or any other string)
        test_return: should the return code be tested (relies on the computation of the return code from the standard output)
        """
        # get "real" output
        real = subprocess.run(cmd, cwd=tempfile.gettempdir(),
                              capture_output=True, encoding='utf-8')

        # check standard output
        # TODO combine the two tests (the one below) in one fonction
        shell_stdout = shell.read_stdout()
        if test_stdout == 'include':
            if not real.stdout in shell_stdout:
                raise AssertionError('The standard output of the command "{}" does not include the following correct result:\n{}\cmd result in shell:\n{}'.format(
                    ' '.join(cmd), real.stdout, shell_stdout))
        elif test_stdout == 'empty':
            if shell_stdout:
                raise AssertionError(
                    'The standard error of the command "{}" shouldbe empty but contains:\n{}'.format(shell_stderr))

        # check standard output
        shell_stderr = shell.read_stderr()
        if test_stderr == 'include':
            if not real.stderr in shell_stderr:
                raise AssertionError('The standard output of the command "{}" does not include the following correct result:\n{}\cmd result in shell:\n{}'.format(
                    ' '.join(cmd), real.stderr, shell_stderr))
        elif test_stdout == 'empty':
            if shell_stderr:
                raise AssertionError(
                    'The standard error of the command "{}" shouldbe empty but contains:\n{}'.format(shell_stderr))

        # check return code
        if test_return:
            std_returncode = self._get_exit_code_from_stdout(shell_stdout)
            if std_returncode != real.returncode:
                raise AssertionError('The command "{}" should return {} but the shell indicates {}'.format(
                    ' '.join(cmd), real.returncode, std_returncode))

    @test
    def test_simple_foregroundjob(self):
        # sleep is one of the given exemples
        cmd = Cmd(['sleep', '1'])
        shell = Shell(self.shell_exec)
        shell.exec_commands([cmd], timeout=5)

    @test
    def test_successfull_foregroundjob(self):
        # check if command output (stdout, stderr and 0 exit status) is the one expected with ls command
        cmd = Cmd(['ls', '-l', '--all', '--author', '-h', '-i', '-S'])
        shell = Shell(self.shell_exec)
        shell.exec_commands([cmd])
        self._test_command_results(
            cmd, shell, test_stdout='include', test_stderr='empty', test_return=True)

    def test_error_foregroundjob(self, cmd: Cmd):
        # check if command output (stdout, stderr and 0 exit status) is the one expected with ls command
        shell = Shell(self.shell_exec)
        shell.exec_commands([cmd])
        self._test_command_results(
            cmd, shell, test_stdout='notest', test_stderr='include', test_return=True)

    @test
    def test_wrongcmd(self):
        # check if command output (stdout, stderr and 0 exit status) is the one expected with ls command
        str_cmd = 'ffof  cf ee ewpqe pepfiwqnfe ff pife piwfpef pi efqplc c p fpc fpi fip qepi fpiaef pifipewq ipfqepif e pifeq fipqe pifewq pfiewa'
        cmd = Cmd(str_cmd.split(' '))
        shell = Shell(self.shell_exec)
        shell.exec_commands([cmd])
        if not shell.read_stderr():
            raise AssertionError(
                'The command "{}" should return an error but stderr is empty'.format(str_cmd))

    def test_foregroundjobs(self):
        print('--- TESTING FOREGROUND JOBS ---')
        self.test_simple_foregroundjob()

        self.test_wrongcmd()

        self.test_successfull_foregroundjob()

        @test
        def test_error_foregroundjob_1(self):
            self.test_error_foregroundjob(Cmd(
                ['ls', '-l', '--all', '--author', '-h', '-i', '-S', 'thisfileshouldnotexist']))
        test_error_foregroundjob_1(self)

        @test
        def test_error_foregroundjob_2(self):
            self.test_error_foregroundjob(
                Cmd(['stat', 'thisfileshouldnotexist']))
        test_error_foregroundjob_2(self)

    @test
    def test_builtin_exit(self):
        # Cannot test exit because otherwise the shell will exit before some test on it (see execute_shell_command)
        # An empty command is tested instead since the exit command is tested anyway at the end
        shell = Shell(self.shell_exec)
        cmd = Cmd(['exit'])
        shell.exec_command(cmd)
        time.sleep(0.5)  # wait to be sure that command was executed
        if shell.is_running():
            raise AssertionError(
                'Command exit was sent but shell is still running')
        shell.exit()

    @test
    def test_builtin_cd(self):
        # Test existing directory
        dir = tempfile.TemporaryDirectory()
        cmd = Cmd(['cd', dir.name])
        shell = Shell(self.shell_exec, cwd='.')
        shell.exec_command(cmd, timeout=1)
        time.sleep(0.5)  # to be "sure" that the shell command executed
        if dir.name != shell.get_cwd():
            raise AssertionError(
                'Changing directory failed: the directory shouldbe {} but it is {}'.format(dir, shell.get_cwd()))
        shell.exit()

        # Test non-existing directory
        cmd = Cmd(['cd', 'thisfoldershouldnotexist'])
        shell = Shell(self.shell_exec)
        shell.exec_commands([cmd], timeout=1)
        if not shell.read_stderr():
            raise AssertionError(
                'The command "{}" should return an error but stderr is empty'.format(' '.join(cmd)))

    def test_builtin(self):
        print('--- TESTING BUITIN COMMANDS ---')
        self.test_builtin_exit()
        self.test_builtin_cd()


if __name__ == "__main__":

    if len(sys.argv) < 2:
        print_usage()
        exit(1)

    t = Test(sys.argv[1])
    # Empty command
    t.test_builtin()
    t.test_foregroundjobs()

    sys.exit(test_failed)
    # # Long nonsensical command
    # execute_commandon_shell(tp_dir, tp_shell_name, b'ffof  cf ee ewpqe pepfiwqnfe ff pife piwfpef pi efqplc c p fpc fpi fip qepi fpiaef pifipewq ipfqepif e pifeq fipqe pifewq pfiewa')
    # # cd without check it works
    # execute_commandon_shell(tp_dir, tp_shell_name, b'cd ..')
    # # Foreground job (wait)
    # execute_commandon_shell(tp_dir, tp_shell_name, b'sleep 2', 5)
    # # Foreground job
    # execute_commandon_shell(tp_dir, tp_shell_name, b'ls -alh')
    # # Foreground job (wait) exit code
    # execute_commandon_shell(tp_dir, tp_shell_name, b'ls klcklncnowo')
    # # cd + foreground job (test if 'cd' work)
    # execute_commandon_shell(tp_dir, tp_shell_name, b'cd ..\nls -alh')
    # # stdout redirect
    # execute_commandon_shell(tp_dir, tp_shell_name, b'ls -alh > ls.out', 3, 'ls.out')
    # # stdout redirect and overwrite (should have same output as before)
    # execute_commandon_shell(tp_dir, tp_shell_name, b'ls -alh > ls.out', 3, 'ls.out')
    # # Pipe
    # execute_commandon_shell(tp_dir, tp_shell_name, b'ls -alh | wc -l')
    # # Background job where shell exit right after
    # execute_commandon_shell(tp_dir, tp_shell_name, b'sleep 2 &', 5)
    # # Background job where shell wait too
    # execute_commandon_shell(tp_dir, tp_shell_name, b'sleep 2 &\nsleep 3', 6)
    # # Background job exit code
    # execute_commandon_shell(tp_dir, tp_shell_name, b'ls clkscncqp &')
    # # Background job SIGTERM (should be ignored)
    # #execute_commandon_shell(tp_dir, tp_shell_name, b'sleep 2 &\nsleep 1\nkill -SIGTERM {pid}', 6)
    # # Background job SIGQUIT (should be ignored)
    # #execute_commandon_shell(tp_dir, tp_shell_name, b'sleep 2 &\nsleep 1\nkill -SIGQUIT {pid}', 6)
    # # Background job SIGHUP
    # #execute_commandon_shell(tp_dir, tp_shell_name, b'sleep 10 &\nsleep 1\nkill -SIGHUP {pid}', 6)
