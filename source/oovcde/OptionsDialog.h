/*
 * OptionsDialog.h
 *
 *  Created on: Jul 11, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef OPTIONSDIALOG_H_
#define OPTIONSDIALOG_H_

#include "Builder.h"
#include "ClassGraph.h"
#include "Gui.h"
#include "PackagesDialogs.h"

class OptionsDialog
    {
    public:
	OptionsDialog();
	virtual ~OptionsDialog()
	    {}
	void openedProject();
	void showScreen();
	void saveScreen();
	void setBuildConfig();
	void newConfig();
	void addConfig();
	void runPackagesDialog();
	// This class calls this function after the options file is written.
	virtual void updateOptions() = 0;
	virtual void buildConfig(char const * const name) = 0;

    private:
	GuiList mBuildConfigList;
	std::string mCurrentBuildConfig;
	int mDialogRunning;
	std::vector<GtkWidget *> mMenuItems;
	void updateBuildConfig();
	void updateBuildMenu(std::vector<std::string> &names);
    };

class ClassPreferencesDialog
    {
    public:
	bool run(Builder &builder, ClassNodeDrawOptions &options);
    };

#endif /* OPTIONSDIALOG_H_ */
