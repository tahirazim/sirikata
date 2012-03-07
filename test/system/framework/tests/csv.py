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

    '''
    @param {String} dirtyFolderName The testManager guarantees that a
    folder exists with this name that csvTest can write arbitrary
    files to without interfering with anything else that's running.
    The testManager will delete this folder after runTest returns.

    @param {String} binPath should be the name of the
    directory that cppoh is in.

    @param {String} cppohBinName The actual name of the
    binary/executale to run.

    '''
    def runTest(self, dirtyFolderName, binPath, cppohBinName, spaceBinName, output=sys.stdout):
        dbFilename = os.path.join(dirtyFolderName, 'unit_test_csv_db.db')
        cppoh_output_filename = os.path.join(dirtyFolderName, 'cppoh.log')
        space_output_filename = os.path.join(dirtyFolderName, 'space.log')

        procs = ProcSet()

        # Random port to avoid conflicts
        port = random.randint(2000, 3000)
        # Space
        space_cmd = [os.path.join(binPath, spaceBinName)]
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
        cppoh_cmd = [os.path.join(binPath, cppohBinName)]
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
        procs.wait(until=self.duration, killAt=self.duration+10, output=output)

        # Print a notification if we had to kill this process
        if procs.hupped():
            print(file=cppoh_output)
            print('UNIT_TEST_TIMEOUT', file=cppoh_output)

        space_output.close()
        cppoh_output.close()

        return self.analyzeOutput(
            analyze=cppoh_output_filename,
            report={ 'Object Host': cppoh_output_filename, 'Space Server': space_output_filename},
            returnCode=procs.returncode(), output=output
            )


    def analyzeOutput(self, analyze, report, returnCode, output):
        success = super(CSVTest, self).analyzeOutput(analyze, returnCode, output=output)
        if not success:
            print("Execution Log:", file=output)
            for report_name, report_file in report.iteritems():
                print("  ", report_name, file=output)
                fp = open(report_file, 'r')
                for line in fp.readlines():
                    print("    ", line, end='', file=output)
                fp.close()
                print(file=output)
        return success
