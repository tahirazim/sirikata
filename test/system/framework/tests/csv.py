#!/usr/bin/python

from __future__ import print_function
import sys

from framework.tests.test import Test
from framework.db.csv import CSVGenerator
import os
import subprocess
from framework.procset import ProcSet
import random

class CSVTest(Test):

    script_paths = None
    duration = 60
    entities = []

    # Some tests currently need to get the timeout hup() call, so we
    # don't want to mark them as failed because of it. This toggles
    # that option.
    needs_hup = False

    def runTest(self):
        dbFilename = os.path.join(self._folder, 'unit_test_csv_db.db')
        cppoh_output_filename = os.path.join(self._folder, 'cppoh.log')
        space_output_filename = os.path.join(self._folder, 'space.log')

        procs = ProcSet()

        # Random port to avoid conflicts
        port = random.randint(2000, 3000)
        # Space
        space_cmd = [ self._binaries['space'] ]
        space_cmd.append('--servermap-options=--port=' + str(port))

        space_output = open(space_output_filename, 'w')
        print(' '.join(space_cmd), file=space_output)
        space_output.flush()
        procs.process(space_cmd, stdout=space_output, stderr=subprocess.STDOUT, wait=False)
        # Wait for space to startup
        procs.sleep(3)


        # OH - create the db file to read from.
        csvGen = CSVGenerator(self.entities);
        csvGen.write(dbFilename)
        cppoh_cmd = [ self._binaries['cppoh'] ]
        cppoh_cmd.append('--servermap-options=--port=' + str(port))
        cppoh_cmd.append('--object-factory=csv')
        cppoh_cmd.append('--object-factory-opts=--db='+ os.path.abspath(dbFilename))
        if self.script_paths:
            cppoh_cmd.append('--objecthost=--scriptManagers=js:{--import-paths=%s}' % (','.join(self.script_paths)))

        cppoh_output = open(cppoh_output_filename,'w')
        print(' '.join(cppoh_cmd), file=cppoh_output)
        cppoh_output.flush()
        procs.process(cppoh_cmd, stdout=cppoh_output, stderr=subprocess.STDOUT, default=True)

        # This type of test expects things to exit cleanly
        procs.wait(until=self.duration, killAt=self.duration+10, output=self.output)

        # Print a notification if we had to kill this process
        if (self.needs_hup and procs.killed()) or (not self.needs_hup and procs.hupped()):
            self.fail('Timed out')

        space_output.close()
        cppoh_output.close()
        self.report_files = { 'Object Host': cppoh_output_filename, 'Space Server': space_output_filename}

        self.assertLogErrorFree(cppoh_output_filename)
        self.assertReturnCode(procs.returncode())

    def report(self):
        print("Execution Log:", file=self.output)
        for report_name, report_file in self.report_files.iteritems():
            print("  ", report_name, file=self.output)
            fp = open(report_file, 'r')
            for line in fp.readlines():
                print("    ", line, end='', file=self.output)
            fp.close()
            print(file=self.output)
