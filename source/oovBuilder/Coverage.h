/*
 * coverage.h
 *
 *  Created on: Sep 17, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef COVERAGE_H_
#define COVERAGE_H_

#include "Project.h"

/// This uses the project file path to read the oov project file and creates
/// a new project file for coverage. It also copies the component types
/// file and packages file to the coverage project.
bool makeCoverageBuildProject(ProjectReader &project);

/// This uses the coverage header file that was generated by oovCovInstr to
/// get all of the source file names and number of instrumented lines in each file.
/// Then it gets the OovCoverageCounts.txt file that matches the total
/// number of instrumented lines to output the percentage of coverage in one file,
/// and to update each source file coverage counts for each set of statements.
bool makeCoverageStats();

#endif /* COVERAGE_H_ */
