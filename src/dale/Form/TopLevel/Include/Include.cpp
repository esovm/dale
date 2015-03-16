#include "Include.h"
#include "Config.h"
#include "../../../Units/Units.h"
#include "../../../CommonDecl/CommonDecl.h"
#include "../../../Node/Node.h"

using namespace dale::ErrorInst;

namespace dale
{
bool
FormTopLevelIncludeParse(Units *units, Node *node)
{
    Context *ctx = units->top()->ctx;

    if (!ctx->er->assertArgNums("include", node, 1, 1)) {
        return false;
    }

    std::vector<Node *> *lst = node->list;
    Node *path_node = (*lst)[1];
    path_node = units->top()->mp->parsePotentialMacroCall(path_node);
    if (!path_node) {
        return false;
    }
    if (!ctx->er->assertArgIsAtom("include", path_node, "1")) {
        return false;
    }
    if (!ctx->er->assertAtomIsStringLiteral("include", path_node, "1")) {
        return false;
    }

    /* Check if the file exists in the current directory, or in
     * ./include.  If it doesn't, go through each of the -I
     * (include_directory_paths) directories.  If it doesn't exist in
     * any of those, check DALE_INCLUDE_PATH, which is set at compile
     * time.  Otherwise, report an error and return nothing. */

    std::string path_buf;

    std::vector<const char *> include_paths;
    include_paths.push_back("");
    include_paths.push_back("./include/");
    std::copy(units->mr->include_directory_paths->begin(),
              units->mr->include_directory_paths->end(),
              back_inserter(include_paths));
    std::string standard_include_path(DALE_INCLUDE_PATH);
    standard_include_path.append("/");
    include_paths.push_back(standard_include_path.c_str());

    FILE *include_file = NULL;
    for (std::vector<const char *>::iterator b = include_paths.begin(),
                                             e = include_paths.end();
            b != e;
            ++b) {
        path_buf.clear();
        path_buf.append((*b));
        path_buf.append(path_node->token->str_value.c_str());
        include_file = fopen(path_buf.c_str(), "r");
        if (include_file) {
            break;
        }
    }

    if (!include_file) {
        Error *e = new Error(FileError, path_node, path_buf.c_str(),
                             strerror(errno));
        ctx->er->addError(e);
        return false;
    }

    Unit *unit = new Unit(path_buf.c_str(), units, ctx->er, ctx->nt,
                          ctx->tr, units->top()->ee,
                          units->top()->is_x86_64);
    units->push(unit);
    units->top()->once_tag.clear();

    units->top()->ee->addModule(units->top()->module);
    CommonDecl::addVarargsFunctions(unit);

    if (!units->no_common) {
        if (units->no_dale_stdlib) {
            units->top()->addCommonDeclarations();
        } else {
            std::vector<const char*> import_forms;
            units->mr->run(ctx, units->top()->module, nullNode(), "drt",
                           &import_forms);
        }
    }

    units->top()->ctx->regetPointers(units->top()->module);

    return true;
}
}
