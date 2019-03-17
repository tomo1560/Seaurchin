﻿#ifndef SCRIPTBUILDER_H
#define SCRIPTBUILDER_H

//---------------------------
// Compilation settings
//

// Set this flag to turn on/off metadata processing
//  0 = off
//  1 = on
#ifndef AS_PROCESS_METADATA
#define AS_PROCESS_METADATA 1
#endif

// TODO: Implement flags for turning on/off include directives and conditional programming



//---------------------------
// Declaration
//

BEGIN_AS_NAMESPACE

class CWScriptBuilder;

// This callback will be called for each #include directive encountered by the
// builder. The callback should call the AddSectionFromFile or AddSectionFromMemory
// to add the included section to the script. If the include cannot be resolved
// then the function should return a negative value to abort the compilation.
typedef int(*WINCLUDECALLBACK_t)(const wchar_t *include, const wchar_t *from, CWScriptBuilder *builder, void *userParam);

// Helper class for loading and pre-processing script files to
// support include directives and metadata declarations
class CWScriptBuilder {
public:
    CWScriptBuilder();

    // Start a new module
    int StartNewModule(asIScriptEngine *engine, const char *moduleName);

    // Load a script section from a file on disk
    // Returns  1 if the file was included
    //          0 if the file had already been included before
    //         <0 on error
    int AddSectionFromFile(const wchar_t *filename);

    // Load a script section from memory
    // Returns  1 if the section was included
    //          0 if a section with the same name had already been included before
    //         <0 on error
    int AddSectionFromMemory(const char *sectionName,
        const char *scriptCode,
        unsigned int scriptLength = 0,
        int lineOffset = 0);

    // Build the added script sections
    int BuildModule();

    // Returns the current module
    asIScriptModule *GetModule();

    // Register the callback for resolving include directive
    void SetIncludeCallback(WINCLUDECALLBACK_t callback, void *userParam);

    // Add a pre-processor define for conditional compilation
    void DefineWord(const char *word);

    // Enumerate included script sections
    unsigned int GetSectionCount() const;
    std::string  GetSectionName(unsigned int idx) const;

#if AS_PROCESS_METADATA == 1
    // Get metadata declared for class types and interfaces
    const char *GetMetadataStringForType(int typeId);

    // Get metadata declared for functions
    const char *GetMetadataStringForFunc(asIScriptFunction *func);

    // Get metadata declared for global variables
    const char *GetMetadataStringForVar(int varIdx);

    // Get metadata declared for class variables
    const char *GetMetadataStringForTypeProperty(int typeId, int varIdx);

    // Get metadata declared for class functions
    const char *GetMetadataStringForTypeMethod(int typeId, asIScriptFunction *method);
#endif

protected:
    void ClearAll();
    int  Build();
    int  ProcessScriptSection(const char *script, unsigned int length, const char *sectionname, int lineOffset);
    int  LoadScriptSection(const wchar_t *filename);
    bool IncludeIfNotAlreadyIncluded(const wchar_t *filename);

    int  SkipStatement(int pos);

    int  ExcludeCode(int start);
    void OverwriteCode(int start, int len);

    asIScriptEngine           *engine;
    asIScriptModule           *module;
    std::string                modifiedScript;

    WINCLUDECALLBACK_t  includeCallback;
    void              *callbackParam;

#if AS_PROCESS_METADATA == 1
    int  ExtractMetadataString(int pos, std::string &outMetadata);
    int  ExtractDeclaration(int pos, std::string &outDeclaration, int &outType);

    // Temporary structure for storing metadata and declaration
    struct SMetadataDecl {
        SMetadataDecl(std::string m, std::string d, int t, std::string c, std::string ns) : metadata(m), declaration(d), type(t), parentClass(c), nameSpace(ns) {}
        std::string metadata;
        std::string declaration;
        int         type;
        std::string parentClass;
        std::string nameSpace;
    };
    std::vector<SMetadataDecl> foundDeclarations;
    std::string currentClass;
    std::string currentNamespace;

    // Storage of metadata for global declarations
    std::map<int, std::string> typeMetadataMap;
    std::map<int, std::string> funcMetadataMap;
    std::map<int, std::string> varMetadataMap;

    // Storage of metadata for class member declarations
    struct SClassMetadata {
        SClassMetadata(const std::string& aName) : className(aName) {}
        std::string className;
        std::map<int, std::string> funcMetadataMap;
        std::map<int, std::string> varMetadataMap;
    };
    std::map<int, SClassMetadata> classMetadataMap;

#endif

    // On Windows the filenames are case insensitive so the comparisons to
    // avoid duplicate includes must also be case insensitive. True case insensitive
    // is not easy as it must be language aware, but a simple implementation such
    // as strcmpi should suffice in almost all cases.
    //
    // ref: http://www.gotw.ca/gotw/029.htm
    // ref: https://msdn.microsoft.com/en-us/library/windows/desktop/dd317761(v=vs.85).aspx
    // ref: http://site.icu-project.org/

    // TODO: Strings by default are treated as UTF8 encoded. If the application choses to
    //       use a different encoding, the comparison algorithm should be adjusted as well

    struct ci_less {
        bool operator()(const std::wstring &a, const std::wstring &b) const
        {
            return wcscmp(a.c_str(), b.c_str()) < 0;
        }
    };
    std::set<std::wstring, ci_less> includedScripts;

    std::set<std::string>      definedWords;
};

END_AS_NAMESPACE

#endif
