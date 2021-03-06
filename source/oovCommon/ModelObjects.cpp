/*
 * ModelObject.cpp
 *
 *  Created on: Jun 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "ModelObjects.h"
#include "OovString.h"
#include "Debug.h"
#include "OovError.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <algorithm>


#define DEBUG_OPER 0
#define DEBUG_TYPES 0


OovStringRef const Visibility::asUmlStr() const
    {
    char const *str = "";
    switch(vis)
        {
        case Visibility::Public:        str = "+";      break;
        case Visibility::Protected:     str = "#";      break;
        case Visibility::Private:       str = "-";      break;
        default:        break;
        }
    return str;
    }

Visibility::Visibility(OovStringRef const umlStr)
    {
    switch(umlStr.getStr()[0])
        {
        default:        // fall through
        case '+':       vis = Visibility::Public;       break;
        case '#':       vis = Visibility::Protected;    break;
        case '-':       vis = Visibility::Private;      break;
        }
    }

class ModelClassifier *ModelType::getClass(ModelType *modelType)
    {
    ModelClassifier *cl = nullptr;
    if(modelType)
        {
        if(modelType->getDataType() == DT_Class)
            cl = static_cast<ModelClassifier *>(modelType);
        }
    return cl;
    }

class ModelClassifier const *ModelType::getClass(ModelType const *modelType)
    {
    ModelClassifier const *cl = nullptr;
    if(modelType)
        {
        if(modelType->getDataType() == DT_Class)
            cl = static_cast<ModelClassifier const *>(modelType);
        }
    return cl;
    }

bool ModelType::isTemplateDefType(OovString const &name)
    {
    return(name.find("<<templ") != std::string::npos);
    }

bool ModelType::isTypedefType(OovString const &name)
    {
    return(name.find("<<type") != std::string::npos);
    }

bool ModelType::isTemplateDefType() const
    {
    return(isTemplateDefType(getName()));
    }

bool ModelType::isTypedefType() const
    {
    return(isTypedefType(getName()));
    }

bool ModelTypeRef::match(ModelTypeRef const &typeRef) const
    {
    return(getDeclType() == typeRef.getDeclType() &&
            isConst() == typeRef.isConst() && isRefer() == typeRef.isRefer());
    }

const class ModelClassifier *ModelDeclarator::getDeclClassType() const
    {
    return getDeclType() ? ModelClassifier::getClass(getDeclType()) : nullptr;
    }


/// Get the position of the right side of the expression.  This will remove
/// left class names, and return the member of function name of the class.
/// Returns zero if no separator was found.
static size_t getRightSidePosFromMemberRefExpr(std::string &expr, bool afterSep)
    {
    static char sepChars[]
        {
        '.',
        ':',    // Find end of ::
        '>'     // Find end of ->
        };
    size_t pos = 0;
    for(auto const c : sepChars)
        {
        size_t testPos = expr.rfind(c);
        if((testPos != std::string::npos) && (testPos > pos))
            {
            pos = testPos;
            }
        }
    if(pos != 0)
        {
        pos++;          // This is now after the separator.
        if(!afterSep)
            {
            size_t splitLen = (expr[pos] == '.') ? 1 : 2;
            pos = static_cast<size_t>(pos - splitLen);
            }
        }
    return pos;
    }

void ModelStatement::eraseOverloadKey(std::string &name)
    {
    size_t pos = name.find(ModelStatement::getOverloadKeySep());
    if(pos != std::string::npos)
        {
        name.erase(pos);
        }
    }

OovString ModelStatement::getOverloadFuncName() const
    {
    OovString opName = getName();

    size_t pos = getRightSidePosFromMemberRefExpr(opName, true);
    if(pos != 0)
        opName.erase(0, pos);
    return opName;
    }

OovString ModelStatement::getFuncName() const
    {
    OovString opName = getOverloadFuncName();
    ModelStatement::eraseOverloadKey(opName);
    return opName;
    }

OovString ModelStatement::getAttrName() const
    {
    OovString attrName = getName();
    if(mStatementType == ST_Call)
        {
        size_t pos = getRightSidePosFromMemberRefExpr(attrName, false);
        if(pos != 0)
            attrName.erase(pos);
        else
            attrName.clear();
        }
    return attrName;
    }

bool ModelStatement::hasBaseClassRef() const
    {
    OovString attrName = getName();
    bool present = attrName.find(getBaseClassMemberRefSep()) != std::string::npos;
    if(!present)
        {
        present = attrName.find(getBaseClassMemberCallSep()) != std::string::npos;
        }
    return(present);
    }

bool ModelStatements::checkAttrUsed(ModelClassifier const *cls,
        OovStringRef attrName) const
    {
    bool used = false;
    for(auto const &stmt : *this)
        {
        ModelType const *modelType = stmt.getClassDecl().getDeclType();
        ModelClassifier const *classifier = ModelType::getClass(modelType);
        if(cls == classifier)
            {
            eModelStatementTypes stateType = stmt.getStatementType();
            if(stateType == ST_VarRef)
                {
                if(stmt.getAttrName() == attrName.getStr())
                    {
                    used = true;
                    break;
                    }
                }
            else if(stateType == ST_Call)
                {
                if(stmt.getAttrName() == attrName.getStr())
                    {
                    used = true;
                    break;
                    }
                }
            }
        }
    return used;
    }

/// @todo - This is duplicate code.
static unsigned int makeHash(OovStringRef const text)
    {
    // djb2 hash function
    unsigned int hash = 5381;
    char const *str = text;

    while(*str)
        {
        int c = *str++;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }
    return hash;
    }

OovString ModelStatement::makeOverloadKeyFromOperUSR(OovStringRef operStr)
    {
    OovString sym;
#define DEBUG_KEY 0
#if(DEBUG_KEY)
    sym = operStr;
    // Remove symbols that conflict with either the CompoundValue class or
    // ModelStatement name separator characters.
    sym.replaceStrs("@", "-");
    sym.replaceStrs("#", "-");
    sym.replaceStrs(":", "-");
    sym.appendInt(makeHash(operStr), 16);
#else
    sym.appendInt(makeHash(operStr), 16);
#endif
    return sym;
    }

bool ModelStatement::compareFuncNames(OovStringRef operName1,
        OovStringRef operName2)
    {
    OovString opName1 = operName1;
    OovString opName2 = operName2;
    if(opName1.find(ModelStatement::getOverloadKeySep()) == std::string::npos ||
            opName2.find(ModelStatement::getOverloadKeySep()) == std::string::npos)
        {
        ModelStatement::eraseOverloadKey(opName1);
        ModelStatement::eraseOverloadKey(opName2);
        }
    return(opName1 == opName2);
    }

OovString ModelOperation::getOverloadFuncName() const
    {
    OovString name = getName();
    if(mOverloadKey.size() > 0)
        {
        name += ModelStatement::getOverloadKeySep();
        name += mOverloadKey;
        }
    return name;
    }

ModelFuncParam *ModelOperation::addMethodParameter(const std::string &name, const ModelType *type,
    bool isConst)
    {
    ModelFuncParam *param = new ModelFuncParam(name, type);
    param->setConst(isConst);
    /// @todo - use make_unique when supported.
    addMethodParameter(std::unique_ptr<ModelFuncParam>(param));
    return param;
    }

ModelBodyVarDecl *ModelOperation::addBodyVarDeclarator(const std::string &name, const ModelType *type,
    bool isConst, bool isRef)
    {
    ModelBodyVarDecl *decl = new ModelBodyVarDecl(name, type);
    decl->setConst(isConst);
    decl->setRefer(isRef);
    /// @todo - use make_unique when supported.
    mBodyVarDeclarators.push_back(std::unique_ptr<ModelBodyVarDecl>(decl));
    return decl;
    }

bool ModelOperation::isDefinition() const
    {
    return(mStatements.size() > 0);
    }

// DEAD CODE
/*
void ModelClassifier::clearAttributes()
    {
    mAttributes.clear();
    }

void ModelClassifier::clearOperations()
    {
    mOperations.clear();
    }
*/

bool ModelClassifier::isDefinition() const
    {
    return(getAttributes().size() + getOperations().size() > 0);
    }

bool ModelClassifier::isOperOverloaded(OovStringRef operName) const
    {
    int count = std::count_if(mOperations.begin(), mOperations.end(),
            [operName](const std::unique_ptr<ModelOperation> &oper)
                { return(oper.get()->getName().compare(operName) == 0); }
        );
    return(count > 1);
    }

ModelAttribute *ModelClassifier::addAttribute(const std::string &name,
        ModelType const *attrType, Visibility scope)
    {
    ModelAttribute *attr = new ModelAttribute(name, attrType, scope);
    /// @todo - use make_unique when supported.
    addAttribute(std::unique_ptr<ModelAttribute>(attr));
    return attr;
    }

ModelOperation *ModelClassifier::addOperation(const std::string &name,
        Visibility access, bool isConst, bool isVirtual)
    {
    ModelOperation *oper = new ModelOperation(name, access, isConst, isVirtual);
    addOperation(std::unique_ptr<ModelOperation>(oper));
    return oper;
    }

// DEAD CODE
/*
void ModelClassifier::removeOperation(ModelOperation *oper)
    {
    size_t index = NoIndex;
    for(size_t i=0; i<mOperations.size(); ++i)
        {
        if(oper == mOperations[i].get())
            index = i;
        }
    if(index != NoIndex)
        {
        delete oper;
        mOperations.erase(mOperations.begin() +
            static_cast<int>(index));
        }
    }
*/

size_t ModelClassifier::getAttributeIndex(const std::string &name) const
    {
    size_t index = NoIndex;
    for(size_t i=0; i<mAttributes.size(); ++i)
        {
        if(mAttributes[i]->getName().compare(name) == 0)
            {
            index = i;
            break;
            }
        }
    return index;
    }

const ModelAttribute *ModelClassifier::getAttribute(const std::string &name) const
    {
    ModelAttribute *attr = nullptr;
    size_t index = getAttributeIndex(name);
    if(index != NoIndex)
        {
        attr = mAttributes[index].get();
        }
    return attr;
    }

size_t ModelClassifier::findExactMatchingOperationIndex(
        OovStringRef overFunc) const
    {
    size_t index = NoIndex;
    for(size_t i=0; i<mOperations.size(); i++)
        {
        ModelOperation const &arrayOp = *mOperations[i];
        if(ModelStatement::compareFuncNames(arrayOp.getOverloadFuncName(), overFunc))
            {
            index = i;
            break;
            }
        }
    return index;
    }

const ModelOperation *ModelClassifier::findExactMatchingOperation(
        OovStringRef overloadFuncName) const
    {
    ModelOperation *oper = nullptr;
    size_t index = findExactMatchingOperationIndex(overloadFuncName);
    if(index != NoIndex)
        {
        oper = mOperations[index].get();
        }
    return oper;
    }

const ModelOperation *ModelClassifier::getOperationByName(OovStringRef const name,
    bool isConst) const
    {
    const ModelOperation *oper = nullptr;
    for(size_t i=0; i<mOperations.size(); i++)
        {
        if(mOperations[i]->getName().compare(name) == 0 &&
                mOperations[i]->isConst() == isConst)
            {
            oper = mOperations[i].get();
            break;
            }
        }
    return oper;
    }

std::vector<const ModelOperation*> ModelClassifier::getOperationsByName(
        OovStringRef const name) const
    {
    std::vector<const ModelOperation*> operations;
    for(size_t i=0; i<mOperations.size(); i++)
        {
        if(mOperations[i]->getName().compare(name) == 0)
            {
            operations.push_back(mOperations[i].get());
            }
        }
    return operations;
    }

void ModelData::clear()
    {
    mModules.clear();
    mAssociations.clear();
    mTypes.clear();
    }

void ModelData::dumpTypes()
    {
#if(DEBUG_TYPES)
    FILE *fp=fopen("DebugTypes.txt", "a");
    if(fp)
        {
        fprintf(fp, "-----\n");
        for(auto &type : mTypes)
            {
            fprintf(fp, "%s %d\n", type->getName().getStr(), type->getModelId());
            }
        fclose(fp);
        }
#endif
    }

const ModelType *ModelData::getTypeRef(OovStringRef const typeName) const
    {
    OovString baseTypeName = getBaseType(typeName);
    return findType(baseTypeName);
    }

ModelType *ModelData::createOrGetTypeRef(OovStringRef const typeName, eModelDataTypes dtype)
    {
    std::string baseTypeName = getBaseType(typeName);
    ModelType *type = findType(baseTypeName);
    if(!type)
        {
        type = static_cast<ModelType*>(createDataType(dtype, baseTypeName));
        }
    return type;
    }

ModelType *ModelData::createTypeRef(OovStringRef const typeName, eModelDataTypes dtype)
    {
    std::string baseTypeName = getBaseType(typeName);
    return static_cast<ModelType*>(createDataType(dtype, baseTypeName));
    }

ModelObject *ModelData::createDataType(eModelDataTypes type, const std::string &id)
    {
    ModelObject *obj = nullptr;
    switch(type)
        {
        case DT_DataType:
            {
            ModelType *dataType = new ModelType(id);
            /// @todo - use make_unique when supported.
            addType(std::unique_ptr<ModelType>(dataType));
            obj = dataType;
            }
            break;

        case DT_Class:
            {
            ModelClassifier *classifier = new ModelClassifier(id);
            /// @todo - use make_unique when supported.
            addType(std::unique_ptr<ModelType>(classifier));
            obj = classifier;
            break;
            }

        default:
            break;
        }
    return obj;
    }

#define BASESPEED 1
#if(BASESPEED)
static inline bool skipStr(char const * * const str, OovStringRef const compareStr)
    {
    char const *needle = compareStr;
    char const *haystack = *str;
    while(*needle)
        {
        if(*needle != *haystack)
            break;
        needle++;
        haystack++;
        }
    bool match = (*needle == '\0');
    if(match)
        *str += needle-compareStr;
    return match;
    }
#endif

#if(BASESPEED)
static bool isIdentC(char c)
    {
    return(isalnum(c) || c == '_');
    }

static char getTokenType(char c)
    {
    if(isIdentC(c))
        c = 'i';
    return c;
    }

static char const *skipWhite(OovStringRef const s)
    {
    char const *p = s;
    while(p)
        {
        if(!isspace(*p))
            break;
        p++;
        }
    return p;
    }
#endif

std::string ModelData::getBaseType(OovStringRef const fullStr)
{
#if(BASESPEED)
    std::string str;
    char const *p = fullStr;
    // Skip leading spaces.
    while(*p == ' ')
        p++;
    while(*p)
        {
        switch(*p)
            {
            case 'c':
                if(!skipStr(&p, "const ") && !skipStr(&p, "class "))
                    str += *p++;
                break;
            case 'm':
                if(!skipStr(&p, "mutable "))
                    str += *p++;
                break;
            case 's':
                if(!skipStr(&p, "struct "))
                    str += *p++;
                break;
            case '*':   p++;
                break;
            case '&':   p++;
                break;
            case ' ':
                {
                char prevTokenType = getTokenType(str[str.length()-1]);
                char nextTokenType = getTokenType(*skipWhite(p));
                if(prevTokenType != nextTokenType)
                    p++;
                else
                    str += *p++;
                }
                break;
            default:    str += *p++;
                break;
            }
        }
    size_t len = str.length()-1;
    if(str[len] == ' ')
        str.resize(len);
#else
    std::string str = fullStr;
    strReplace("const ", " ", str);
    strReplace("class ", " ", str);
    strReplace("mutable ", " ", str);
    strReplace("*", "", str);
    strReplace("&", "", str);
    strRemoveSpaceAround("<", str);
    strRemoveSpaceAround(">", str);     // There are two spaces near >
    strRemoveSpaceAround(">", str);
    strRemoveSpaceAround(":", str);
    if(str[0] == ' ')
        str.erase(0, 1);
    int lastPos = str.length()-1;
    if(str[lastPos] == ' ')
        str.erase(lastPos, 1);
#endif
    return str;
}

static inline bool compareStrs(OovStringRef const tstr1, OovStringRef const tstr2)
    {
    return (strcmp(tstr1, tstr2) < 0);
    }

#define BINARYSPEED 1

void ModelData::addType(std::unique_ptr<ModelType> &&type)
    {
#if(BINARYSPEED)
    std::string baseTypeName = getBaseType(type->getName());
    type->setName(baseTypeName);
    auto it = std::upper_bound(mTypes.begin(), mTypes.end(), baseTypeName,
        [](OovStringRef const mod1Name, std::unique_ptr<ModelType> &mod2) -> bool
        { return(compareStrs(mod1Name, mod2->getName())); } );
    mTypes.insert(it, std::move(type));
#else
    mTypes.push_back(type);
#endif
    }

/*
static OovString getTemplateUseBaseName(OovString const &name)
    {
    OovString templName;
    if(!ModelType::isTypedefType(name))
        {
        size_t pos = name.find('<');
        templName = name.substr(0, pos);
        }
    return templName;
    }
*/

const ModelType *ModelData::findType(OovStringRef const name) const
    {
    const ModelType *type = nullptr;
#if(BINARYSPEED)
    // This comparison must produce the same sort order as addType.
    std::string baseTypeName = getBaseType(name);
    auto iter = std::lower_bound(mTypes.begin(), mTypes.end(), baseTypeName,
        [](std::unique_ptr<ModelType> const &mod1, OovStringRef const mod2Name) -> bool
        { return(compareStrs(mod1->getName(), mod2Name)); } );
    if(iter != mTypes.end())
        {
        if(baseTypeName.compare((*iter)->getName()) == 0)
            type = (*iter).get();
        }
#else
    std::string baseTypeName = getBaseType(name);
    for(auto &iterType : mTypes)
        {
        if(iterType->getName().compare(baseTypeName) == 0)
            {
            type = iterType;
            break;
            }
        }
#endif
    return type;
    }

ModelType *ModelData::findType(OovStringRef const name)
    {
    return const_cast<ModelType*>(static_cast<const ModelData*>(this)->findType(name));
    }

/*
static size_t findIdentC(const std::string &str, size_t pos)
    {
    size_t ret = std::string::npos;
    for(size_t i=pos; i<str.length(); i++)
        {
        if(isIdentC(str[i]))
            {
            ret = i;
            break;
            }
        }
    return ret;
    }

static size_t findNonIdentC(const std::string &str, size_t pos)
    {
    size_t ret = std::string::npos;
    for(size_t i=pos; i<str.length(); i++)
        {
        if(!isIdentC(str[i]))
            {
            ret = i;
            break;
            }
        }
    return ret;
    }

static void getIdents(const std::string &str, std::vector<std::string> &idents)
    {
    size_t startPos=0;
    while(startPos != std::string::npos)
        {
        startPos = findIdentC(str, startPos);
        if(startPos != std::string::npos)
            {
            size_t endPos = findNonIdentC(str, startPos);
            idents.push_back(str.substr(startPos, endPos-startPos));
            if(endPos != std::string::npos)
                startPos = endPos+1;
            else
                startPos = endPos;
            }
        }
    }
*/

bool ConstModelDeclClasses::addUnique(ModelDeclClass const &decl)
    {
    bool added = false;
    auto declIter = std::find_if(begin(), end(),
            [&decl](ModelDeclClass const &existingDecl)
               { return(decl == existingDecl); });
    if(declIter == end())
        {
        added = true;
        push_back(decl);
        }
    return(added);
    }

bool ConstModelClassifierVector::addUnique(const ModelClassifier *cl)
    {
    bool added = false;
    auto clIter = std::find_if(begin(), end(),
            [&cl](ModelClassifier const *existingNode)
               { return(cl == existingNode); });
    if(clIter == end())
        {
        added = true;
        push_back(cl);
        }
    return(added);
    }

void ModelData::getRelatedBodyVarClasses(const ModelClassifier &classifier,
        ConstModelDeclClasses &declClasses) const
    {
    declClasses.clear();
    for(auto &oper : classifier.getOperations())
        {
        for(auto &vd : oper->getBodyVarDeclarators())
            {
            declClasses.addUnique(vd.get());
            }
        // Normally the call statements would not have to be checked since most
        // calls are through either variable declarations, or through
        // the parameters, but calls to global classes cannot be found this way.
        for(auto &stmt : oper->getStatements())
            {
            if(stmt.getStatementType() == ST_Call)
                {
                const ModelClassifier *cl = ModelClassifier::getClass(
                    stmt.getClassDecl().getDeclType());
                if(cl)
                    {
                    declClasses.addUnique(ModelDeclClass(stmt.getAttrName(),
                            stmt.getClassDecl().getDeclType()));
                    }
                }
            }
        }
    }

void ModelData::addBaseClasses(ModelClassifier const &type,
        ConstModelClassifierVector &classes) const
    {
    for(auto const &assoc : mAssociations)
        {
        if(assoc.get()->getChild() == &type)
            {
            ModelClassifier const *parent = assoc.get()->getParent();
            if(classes.addUnique(parent))
                {
                addBaseClasses(*parent, classes);
                }
            }
        }
    }
