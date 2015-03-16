#include "../../../Units/Units.h"
#include "../../../Node/Node.h"
#include "../../../ParseResult/ParseResult.h"
#include "../../../Function/Function.h"
#include "../../../Operation/Destruct/Destruct.h"
#include "../Inst/Inst.h"
#include "../../../llvm_Function.h"

using namespace dale::ErrorInst;

namespace dale
{
bool
FormProcNullParse(Units *units, Function *fn, llvm::BasicBlock *block,
                  Node *node, bool get_address, bool prefixed_with_core,
                  ParseResult *pr)
{
    Context *ctx = units->top()->ctx;

    if (!ctx->er->assertArgNums("null", node, 1, 1)) {
        return false;
    }

    std::vector<Node *> *lst = node->list;
    Node *node_value = (*lst)[1];

    /* Take the second value, call parsePotentialMacroCall, and see if
     * it's a list.  If it is a list, and the form is : or $, then
     * show an error about that value never being null. */

    if (node_value->is_list) {
        node_value = units->top()->mp->parsePotentialMacroCall(node_value);
        if (!node_value) {
            return false;
        }
        if (node_value->is_list) {
            Node *first = node_value->list->at(0);
            if (first->is_token) {
                const char *v = first->token->str_value.c_str();
                if ((!strcmp(v, ":")) || (!strcmp(v, "$"))) {
                    Error *e = new Error(ValueWillNeverBeNull, node_value);
                    ctx->er->addError(e);
                    return false;
                }
            }
        }
    }

    ParseResult pr_value;
    bool res = FormProcInstParse(units, fn, block, node_value, false,
                                 false, NULL, &pr_value);

    if (!res) {
        return false;
    }
    if (!ctx->er->assertIsPointerType("null", node_value, pr_value.type, "1")) {
        return false;
    }

    llvm::IRBuilder<> builder(pr_value.block);
    llvm::Value *int_res =
        builder.CreatePtrToInt(pr_value.value, ctx->nt->getNativeIntType());

    llvm::Value *null_res =
        llvm::cast<llvm::Value>(
            builder.CreateICmpEQ(int_res, ctx->nt->getLLVMZero())
        );

    ParseResult pr_destruct;
    res = Operation::Destruct(ctx, &pr_value, &pr_destruct);
    if (!res) {
        return false;
    }

    pr->set(pr_destruct.block, ctx->tr->type_bool, null_res);

    return true;
}
}
