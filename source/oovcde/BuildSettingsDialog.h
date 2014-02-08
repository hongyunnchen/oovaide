/*
 * BuildSettingsDialog.h
 *
 *  Created on: Sep 8, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef BUILDSETTINGSDIALOG_H_
#define BUILDSETTINGSDIALOG_H_

#include "Builder.h"
#include "ClassGraph.h"
#include "Project.h"
#include "Components.h"
#include "Gui.h"

class BuildSettingsDialog
    {
    public:
	BuildSettingsDialog();
	virtual ~BuildSettingsDialog()
	    {}
	void enterScreen();
	void saveScreen();
	void switchComponent();

    private:
	GuiList mComponentList;
	ComponentTypesFile mComponentFile;
	std::string mLastCompName;
	void saveFromScreen(std::string const &compName);
	void loadToScreen(std::string const &compName);
    };


#endif /* BUILDSETTINGSDIALOG_H_ */
