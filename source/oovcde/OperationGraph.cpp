/*
 * OperationGraph.cpp
 *
 *  Created on: Jul 24, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "OperationGraph.h"
#include <algorithm>
#include "Debug.h"

#define DEBUG_OPERGRAPH 0

#if(DEBUG_OPERGRAPH)
static DebugFile sLog("OperGraph.txt");
#endif


OperationCall *OperationStatement::getCall()
    {
    return(mStatementType == ST_Call ? static_cast<OperationCall*>(this) : nullptr);
    }

#if(VAR_REF)
OperationVarRef *OperationStatement::getVarRef()
    {
    return(mStatementType == ST_VarRef ? static_cast<OperationVarRef*>(this) : nullptr);
    }
#endif

const OperationNestStart *OperationStatement::getNestStart() const
    {
    return(mStatementType == ST_OpenNest ?
	    static_cast<const OperationNestStart*>(this) : nullptr);
    }

const OperationNestEnd *OperationStatement::getNestEnd() const
    {
    return(mStatementType == ST_CloseNest ?
	    static_cast<const OperationNestEnd*>(this) : nullptr);
    }

bool OperationCall::compareOperation(const OperationCall &call) const
    {
    return(getOperClassIndex() == call.getOperClassIndex() &&
	    std::string(getName()).compare(call.getName()) == 0 &&
	    isConst() == call.isConst());
    }

int OperationDefinition::getNestDepth() const
    {
    int maxDepth = 0;
    int depth = 0;
    for(const auto &stmt : mStatements)
	{
	if(stmt->getStatementType() == ST_OpenNest)
	    {
	    depth++;
	    if(depth > maxDepth)
		maxDepth = depth;
	    }
	else if(stmt->getStatementType() == ST_CloseNest)
	    {
	    depth--;
	    }
	}
    return maxDepth;
    }

const OperationCall *OperationDefinition::getCall(int x, int y) const
    {
    OperationCall *retCall = NULL;
    for(const auto &stmt : mStatements)
	{
	if(stmt->getStatementType() == ST_Call)
	    {
	    OperationCall *call = stmt->getCall();
	    GraphRect rect;
	    call->getRect(rect);
	    if(rect.isPointIn(GraphPoint(x, y)))
		{
		retCall = call;
		break;
		}
	    }
	}
    return retCall;
    }

bool OperationDefinition::isCalled(const OperationCall &opcall) const
    {
    bool called = false;
    for(const auto &stmt : mStatements)
	{
	if(stmt->getStatementType() == ST_Call)
	    {
	    const OperationCall *stmtcall = stmt->getCall();
	    if(stmtcall->compareOperation(opcall))
		{
		called = true;
		break;
		}
	    }
	}
    return called;
    }

bool OperationDefinition::isClassReferred(int classIndex) const
    {
    bool referred = (getOperClassIndex() == classIndex);
    if(!referred)
	{
	for(const auto &stmt : mStatements)
	    {
	    OperationCall *call = stmt->getCall();
	    if(call && call->getOperClassIndex() == classIndex)
		{
		referred = true;
		break;
		}
	    }
	}
    return referred;
    }


int OperationGraph::addOrGetClass(const ModelClassifier *cls, eGetClass gc)
    {
    int index = -1;
    for(size_t i=0; i<mOpClasses.size(); i++)
	{
	if(mOpClasses[i].getType() == cls)
	    {
	    index = i;
	    break;
	    }
	}
    if(index == -1 && gc == GC_AddClasses)
	{
	mOpClasses.push_back(cls);
	index = mOpClasses.size()-1;
	}
    return index;
    }


/// @todo - these two classes are a bit of a kludge.  Since not all classes
/// are defined in xmi files (such as external libraries), then there is no
/// class definition, but there is still a call to the method in the class.
/// This allows displaying the call even though there is no operation defined
/// in the model.
class DummyOperation:public ModelOperation
    {
    public:
	DummyOperation(const std::string &name, bool isConst):
	    ModelOperation(name, Visibility::Public, isConst)
	    {}
    };

class DummyOperationCall:public OperationCall
    {
    public:
	DummyOperationCall(int operClassIndex, const std::string &name, bool isConst):
	    OperationCall(operClassIndex, *(mDo=new DummyOperation(name, isConst)))
	    {}
	~DummyOperationCall()
	    { delete mDo; }
    private:
	DummyOperation *mDo;
    };

void OperationGraph::fillDefinition(const ModelStatements &stmts, OperationDefinition &opDef,
	eGetClass gc)
    {
    for(auto &stmt : stmts)
	{
	if(stmt.getStatementType() == ST_OpenNest)
	    {
	    opDef.addNestStart(stmt.getName().c_str());
	    }
	else if(stmt.getStatementType() == ST_CloseNest)
	    {
	    opDef.addNestEnd();
	    }
#if(VAR_REF)
	else if(stmt.getStatementType() == ST_VarRef)
	    {
	    const ModelClassifier *cls = stmt.getDecl().getDeclType()->getClass();
	    if(cls)
		{
		int classIndex = addOrGetClass(cls, gc);
		if(classIndex != -1)
		    {
		    const ModelAttribute *targetAttr = cls->getAttribute(stmt.getName());
			{
			opDef.addStatement(std::unique_ptr<OperationStatement>(
			    new OperationVarRef(classIndex, *targetAttr)));
			}
		    }
		}
	    }
#endif
	else if(stmt.getStatementType() == ST_Call)
	    {
	    const ModelClassifier *cls = stmt.getDecl().getDeclType()->getClass();
	    // If there is no class, it must be an [else]
	    if(!cls)
		{
		opDef.addStatement(std::unique_ptr<OperationStatement>(
			new DummyOperationCall(-1, stmt.getName(),
			stmt.getDecl().isConst())));
		}
	    if(cls)
		{
		int classIndex = addOrGetClass(cls, gc);
		if(classIndex != -1)
		    {
		    const ModelOperation *targetOper = cls->getOperationAnyConst(stmt.getName(),
			    stmt.getDecl().isConst());
		    if(targetOper)
			{
			/// @todo - use make_unique when supported.
			opDef.addStatement(std::unique_ptr<OperationStatement>(
				new OperationCall(classIndex, *targetOper)));
    #if(DEBUG_OPERGRAPH)
			fprintf(sLog.mFp, "%d %s\n", classIndex, targetOper->getName().c_str());
    #endif
			}
		    else
			{
			opDef.addStatement(std::unique_ptr<OperationStatement>(
				new DummyOperationCall(classIndex, stmt.getName(),
				stmt.getDecl().isConst())));
    #if(DEBUG_OPERGRAPH)
			fprintf(sLog.mFp, "Bad Oper %d %s %d\n", classIndex,
				call->getName().c_str(), call->getDecl().isConst());
			for(const auto oper : cls->getOperations())
			    {
			    fprintf(sLog.mFp, "  %s %d\n", oper->getName().c_str(),
				    oper->isConst());
			    }
    #endif
			}
		    }
		else
		    {
    #if(DEBUG_OPERGRAPH)
		    fprintf(sLog.mFp, "Bad Class %d\n", classIndex);
    #endif
		    }
		}
	    }
	}
    }

void OperationGraph::addDefinition(int classIndex, const ModelOperation &oper)
    {
    OperationDefinition *opDef = new OperationDefinition(classIndex, oper);
    fillDefinition(oper.getStatements(), *opDef, GC_AddClasses);
    mOperations.push_back(opDef);
    mModified = true;
    }

/// @todo - prevent infinite recursion.
/// @todo - this doesn't work for overloaded functions.
void OperationGraph::addRelatedOperations(const ModelClassifier &sourceClass,
	const ModelOperation &sourceOper,
	eAddOperationTypes addType, int maxDepth)
    {
    -- maxDepth;
    if(maxDepth >= 0)
	{
	int sourceClassIndex = addOrGetClass(&sourceClass, GC_AddClasses);
	addDefinition(sourceClassIndex, sourceOper);
	}
    }

void OperationGraph::clearGraphAndAddOperation(const ModelData &model,
	const OperationDrawOptions &options, char const * const className,
	char const * const operName, bool isConst, int nodeDepth)
    {
#if(DEBUG_OPERGRAPH)
    fprintf(sLog.mFp, "---------\n");
#endif
    clearGraph();
    const ModelClassifier *sourceClass = model.getTypeRef(className)->getClass();
    if(sourceClass)
	{
	const ModelOperation *sourceOper = sourceClass->getOperation(operName, isConst);
	if(sourceOper)
	    {
	    addRelatedOperations(*sourceClass, *sourceOper, OperationGraph::AO_All, 2);
	    }
	}
    mModified = false;
    }

void OperationGraph::removeOperation(int index)
    {
    delete mOperations[index];
    mOperations.erase(mOperations.begin() + index);
    mModified = true;
    }

void OperationGraph::removeClass(int index)
    {
    mOpClasses.erase(mOpClasses.begin() + index);
//    removeUnusedClasses();
    mModified = true;
    }

void OperationGraph::removeNode(const OperationClass *classNode)
    {
    int classIndex = -1;
    for(size_t i=0; i<mOpClasses.size(); i++)
	{
	if(mOpClasses[i].getPosition().x == classNode->getPosition().x)
	    {
	    removeClass(i);
	    classIndex = i;
	    break;
	    }
	}
    if(classIndex != -1)
	{
	for(size_t i=0; i<mOperations.size(); i++)
	    {
	    if(mOperations[i]->getOperClassIndex() == classIndex)
		{
		removeOperation(i);
		break;
		}
	    }
	for(const auto &oper : mOperations)
	    {
	    if(oper->getOperClassIndex() > classIndex)
		oper->decrementClassIndex();
	    oper->removeStatements();
	    fillDefinition(oper->getOperation().getStatements(), *oper, FT_OnlyGetClasses);
	    }
	}
    }

int OperationGraph::getNestDepth(int classIndex)
    {
    int maxDepth = 0;
    for(const auto &oper : mOperations)
	{
	if(oper->getOperClassIndex() == classIndex)
	    {
	    int depth = oper->getNestDepth();
	    if(depth > maxDepth)
		maxDepth = depth;
	    }
	}
    return maxDepth;
    }

const OperationClass *OperationGraph::getNode(int x, int y) const
    {
    const OperationClass *node = nullptr;
    for(size_t i=0; i<mOpClasses.size(); i++)
	{
	GraphRect rect;
	mOpClasses[i].getRect(rect);
	if(rect.isPointIn(GraphPoint(x, y)))
	    node = &mOpClasses[i];
	}
    return node;
    }

const OperationCall *OperationGraph::getOperation(int x, int y) const
    {
    const OperationCall *call = nullptr;
    for(const auto &oper : mOperations)
	{
	GraphRect rect;
	oper->getRect(rect);
	if(rect.isPointIn(GraphPoint(x, y)))
	    {
	    call = oper;
	    break;
	    }
	call = oper->getCall(x, y);
	if(call)
	    break;
	}
    return call;
    }

std::string OperationGraph::getClassName(const OperationCall &opcall) const
    {
    return mOpClasses[opcall.getOperClassIndex()].getType()->getName();
    }

void OperationGraph::addOperDefinition(const OperationCall &opcall)
    {
    const ModelClassifier *cls = mOpClasses[opcall.getOperClassIndex()].
	    getType()->getClass();
    const ModelOperation *targetOper = cls->getOperation(opcall.getName(),
	    opcall.isConst());
    if(!isOperDefined(opcall) && targetOper)
	addDefinition(opcall.getOperClassIndex(), *targetOper);
    }

void OperationGraph::addOperCallers(const ModelStatements &stmts,
	const ModelClassifier &srcCls, const ModelOperation &oper,
	const OperationCall &callee)
    {
    for(auto const &stmt : stmts)
	{
	if(stmt.getStatementType() == ST_Call)
	    {
	    const ModelClassifier *callerCls = stmt.getDecl().getDeclType()->
		    getClass();
	    if((stmt.getName().compare(callee.getName()) == 0) &&
		    callerCls == mOpClasses[callee.getOperClassIndex()].getType())
		{
		addRelatedOperations(srcCls, oper, OperationGraph::AO_All, 1);
		}
	    }
	}
    }

void OperationGraph::addOperCallers(const ModelData &model, const OperationCall &callee)
    {
    for(const auto &type : model.mTypes)
	{
	const ModelClassifier *srcCls = type->getClass();
	if(srcCls)
	    {
	    for(const auto &oper : srcCls->getOperations())
		{
		addOperCallers(oper->getStatements(), *srcCls, *oper, callee);
		}
	    }
	}
    }

void OperationGraph::removeOperDefinition(const OperationCall &opcall)
    {
    const ModelClassifier *cls = mOpClasses[opcall.getOperClassIndex()].getType()->
	    getClass();
    const ModelOperation *targetOper = cls->getOperation(opcall.getName(),
	    opcall.isConst());
    if(targetOper)
	{
	const OperationDefinition *operDef = getOperDefinition(opcall);
	if(operDef)
	    {
	    for(size_t i=0; i<mOperations.size(); i++)
		{
		if(mOperations[i] == operDef)
		    {
		    removeOperation(i);
		    break;
		    }
		}
	    }
	removeUnusedClasses();
	}
    }

void OperationGraph::removeUnusedClasses()
    {
    for(size_t i=0; i<mOpClasses.size(); i++)
	{
	bool used = false;
	for(const auto &oper : mOperations)
	    {
	    if(oper->isClassReferred(i))
		used = true;
	    }
	if(!used)
	    {
	    removeNode(&mOpClasses[i]);
	    i--;
	    }
	}
    }

bool OperationGraph::isOperCalled(const OperationCall &opcall) const
    {
    bool called = false;
    for(const auto &oper : mOperations)
	{
	if(oper->isCalled(opcall))
	    {
	    called = true;
	    break;
	    }
	}
    return called;
    }

OperationDefinition *OperationGraph::getOperDefinition(
	const OperationCall &opcall) const
    {
    OperationDefinition *operDef = NULL;
    for(const auto &oper : mOperations)
	{
	if(oper->getOperClassIndex() == opcall.getOperClassIndex() &&
		std::string(opcall.getName()).compare(oper->getName()) == 0)
	    {
	    operDef = oper;
	    break;
	    }
	}
    return operDef;
    }
