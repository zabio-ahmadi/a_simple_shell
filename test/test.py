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


def test(func):
    def wrapper(self):
        print(func.__name__ + ": ", end="")
        try:
            func(self)
        except Exception as e:
            print(Fore.RED + "FAILED" + Fore.RESET)
            logging.error(f"{type(e).__name__}:{str(e)}")
            return
        print(Fore.GREEN + "SUCCESS" + Fore.RESET)

    return wrapper


class Test:
    def __init__(self, shell_exec: str) -> None:
        self.shell_exec = Path(shell_exec).resolve()


    def _get_exit_code_from_stdout(self, stdout: str) -> int:
        # Find line with keyword "code"
        for line in stdout.splitlines():
            if "code" in line:
                # Assumes that exit code is after keyword code and that it is the only digits
                return int(''.join([c for c in line.split('code')[-1] if c.isdigit()]))
        raise AssertionError('No exit code found')


    def _execute_shell_command(self, command: list[str], cwd: str = tempfile.gettempdir(), duration_cmd: float = 0.2, timeout: int = 3):
        """ Execute a shell command and tries to determine if the command is correctly executed
        """

        # Create a string out of a command
        str_cmd = ' '.join(command)

        # Execute the shell and keep track of the process
        shell_process = subprocess.Popen(
            [self.shell_exec], cwd=cwd, encoding='utf-8',
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        shell_ps = psutil.Process(shell_process.pid)
        try:
            # Execute the command and wait for it to finish
            shell_process.stdin.write(str_cmd + '\n')
            shell_process.stdin.flush()

            #TODO: we wait a bit here to ensure the child is created ? However it seems is it not needed, how to ensure it ?
            time.sleep(0.1)

            # Wait for children to die and check if some are zombies
            # TODO: it can happen that the child is stuck (e.g. if it becomes a shell cause cannot execve) in that case the code bellow blocks
            children = shell_ps.children()
            while(children):
                for c in children:
                    if c.status() == psutil.STATUS_ZOMBIE:
                        raise AssertionError('The shell child process {} is a zombie for command "{}"'.format(c, str_cmd))
                time.sleep(0.1)
                children = shell_ps.children()


            # Check if command process has become a child of process 1 (e.g. by double fork)
            init_ps = psutil.Process(1)
            for c in init_ps.children():
                if c.cmdline() == command:
                    raise AssertionError('The command "{}" seem to be a child of process 1'.format(str_cmd))

            # update current working directory
            cwd = shell_ps.cwd()

            # Terminate shell by using exit command and returning shell outputs
            stdout, stderr = shell_process.communicate(input='exit\n', timeout=timeout)

            # Test if shell is still running
            if shell_ps.is_running():
                raise AssertionError('Shell is still running after en of communication, either exit is not working or the shell does not terminate on Ctrl+D')

            return stdout, stderr, cwd

        except subprocess.TimeoutExpired:
            shell_process.kill()
            print('Timeout during command {}'.format(str_cmd))
            raise subprocess.TimeoutExpired

    @test
    def test_simple_foregroundjob(self):
        # sleep is one of the given exemples
        self._execute_shell_command(['sleep', '1'], timeout=5)


    @test
    def test_successfull_foregroundjob(self):
        # check if command output (stdout, stderr and 0 exit status) is the one expected with ls command
        cmd = ['ls', '-l', '--all', '--author', '-h', '-i', '-S']
        std_stdout, std_stderr, _ = self._execute_shell_command(cmd)

        # get "real" output
        real = subprocess.run(cmd, cwd=tempfile.gettempdir(), capture_output=True, encoding='utf-8')

        # check standard output
        if not real.stdout in std_stdout:
            raise AssertionError('The standard output of the command "{}" does not include the following correct result:\n{}\cmd result in shell:\n{}'.format(' '.join(cmd), real.stdout, std_stdout))

        # check standard error
        if std_stderr:
            raise AssertionError('The standard error of the command "{}" shouldbe empty but contains:\n{}'.format(std_stderr))

        # check return code
        std_returncode = self._get_exit_code_from_stdout(std_stdout)
        if std_returncode != real.returncode:
            raise AssertionError('The command "{}" should return {} but the shell indicates {}'.format(' '.join(cmd), real.returncode, std_returncode))


    def test_error_foregroundjob(self, cmd: list[str]):
        # check if command output (stdout, stderr and 0 exit status) is the one expected with ls command
        std_stdout, std_stderr, _ = self._execute_shell_command(cmd)

        # get "real" output
        real = subprocess.run(cmd, cwd=tempfile.gettempdir(), capture_output=True, encoding='utf-8')

        # check standard output
        if not real.stderr in std_stderr:
            raise AssertionError('The standard output of the command "{}" does not include the following correct result:\n{}\cmd result in shell:\n{}'.format(' '.join(cmd), real.stderr, std_stderr))

        # do not check if stdout is empty because it will contain return code...

        # check return code
        std_returncode = self._get_exit_code_from_stdout(std_stdout)
        if std_returncode != real.returncode:
            raise AssertionError('The command "{}" should return {} but the shell indicates {}'.format(' '.join(cmd), real.returncode, std_returncode))


    @test
    def test_wrongcmd(self):
        # check if command output (stdout, stderr and 0 exit status) is the one expected with ls command
        str_cmd = 'ffof  cf ee ewpqe pepfiwqnfe ff pife piwfpef pi efqplc c p fpc fpi fip qepi fpiaef pifipewq ipfqepif e pifeq fipqe pifewq pfiewa'
        cmd = str_cmd.split(' ')
        _, std_stderr, _ = self._execute_shell_command(cmd)

        if not std_stderr:
            raise AssertionError('The command "{}" should return an error but stderr is empty'.format(str_cmd))


    def test_foregroundjobs(self):
        print('--- TESTING FOREGROUND JOBS ---')
        self.test_simple_foregroundjob()

        self.test_wrongcmd()

        self.test_successfull_foregroundjob()

        @test
        def test_error_foregroundjob_1(self):
            self.test_error_foregroundjob(['ls', '-l', '--all', '--author', '-h', '-i', '-S', 'thisfileshouldnotexist'])
        test_error_foregroundjob_1(self)

        @test
        def test_error_foregroundjob_2(self):
            self.test_error_foregroundjob(['stat', 'thisfileshouldnotexist'])
        test_error_foregroundjob_2(self)


    @test
    def test_builtin_exit(self):
        # Cannot test exit because otherwise the shell will exit before some test on it (see execute_shell_command)
        # An empty command is tested instead since the exit command is tested anyway at the end
        self._execute_shell_command([''], timeout=1)


    @test
    def test_builtin_cd(self):
        # Test existing directory
        dir = tempfile.TemporaryDirectory()
        _, _, cwd = self._execute_shell_command(['cd', dir.name], cwd='.', timeout=1)
        if dir.name != cwd:
            raise AssertionError('Changing directory failed: the directory shouldbe {} but it is {}'.format(dir, cwd))

        # Test non-existing directory
        cmd = ['cd', 'thisfoldershouldnotexist']
        _, stderr, _ = self._execute_shell_command(cmd, timeout=1)
        if not stderr:
            raise AssertionError('The command "{}" should return an error but stderr is empty'.format(' '.join(cmd)))


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
