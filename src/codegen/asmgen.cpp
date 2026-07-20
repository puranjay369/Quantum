#include "asmgen.h"
#include <stdexcept>

std::string AsmGen::generate(ProgramNode* program) {
    out << ".intel_syntax noprefix\n";      // Use Intel syntax for assembly
    out << ".global _start\n";              // Entry point for the program
    out << ".bss\n";                        // Uninitialized data section
    out << "printBuf: .skip 32\n";          // Buffer for printing integers (32 bytes)
    out << ".text\n";                       // Code section

    genProgram(program);

    out << ".data\n";                       // Initialized data section
    out << "divzero_msg: .ascii \"Division by zero\\n\"\n"; 
    out << data.str();

    return out.str();
}   

void AsmGen::genProgram(ProgramNode* node) {
    
    FunctionDefNode *mainFunc = nullptr;
    for ( auto& fn : node->functions ) {
        auto* func = static_cast<FunctionDefNode*>(fn.get());
        if ( func->name == "main" ) {
            mainFunc = func;
            break;
        }
    }
    
    if ( !mainFunc ) {
        throw std::runtime_error("AsmGen: main() function not found");    
    }

    genFunction(mainFunc);
}

void AsmGen::genFunction(FunctionDefNode* node) {
    slots.clear();
    nextOffset = 0;

    out << "_start:\n";        // Entry point for the program
    out << "    push rbp\n";
    out << "    mov rbp, rsp\n";
    out << "    sub rsp, 256\n";   // reserve a generous chunk for now — we'll compute the exact size properly later

    auto* body = static_cast<BlockNode*>(node->body.get());
    for ( auto& stmt : body->statements ) {
        genStatement(stmt.get());
    }   

    out << "    mov rax, 60\n";
    out << "    mov rdi, 0\n";
    out << "    syscall\n";
}

void AsmGen::genStatement(ASTNode* node) {
    if ( node->kind == NodeType::VarDecl) {
        genVarDecl(static_cast<VarDeclNode*>(node));
        return;
    }
    if ( node->kind == NodeType::FunctionCall ) {
        auto* call = static_cast<FunctionCallNode*>(node);
        if ( call->callee != "print" ) {
            throw std::runtime_error("AsmGen: unsupported function call");
        }
        if ( call->args.empty() ) {
            out << "    mov rax, 0\n";        // nothing to load -> just print 0
        } else {
            genExpr(call->args[0].get());     // load the value of the first argument into rax
        }
        genPrintRuntime();
    }
}

void AsmGen::genVarDecl(VarDeclNode* node) {
    nextOffset += 8; // Assuming 8 bytes for int
    slots[node->name] = nextOffset; // Store the stack offset for the variable

    genExpr(node->initializer.get()); // Load the initializer value into rax
    out << "    mov qword ptr [rbp-" << nextOffset << "], rax\n";
}

void AsmGen::genExpr(ASTNode* node) {
    if ( node->kind == NodeType::IntLiteral ) {
        auto* intLit = static_cast<IntLiteralNode*>(node);
        out << "    mov rax, " << intLit->value << "\n";
    } else if ( node->kind == NodeType::Identifier ) {
        auto* id = static_cast<IdentifierNode*>(node);
        auto iter = slots.find(id->name);
        if ( iter == slots.end() ) { // Variable not found in current scope
            throw std::runtime_error("AsmGen: variable '" + id->name + "' not found in current scope.");
        }
        int offset = iter->second; // Get the stack offset for the variable
        out << "    mov rax, qword ptr [rbp-" << offset << "]\n";
    } else if ( node->kind == NodeType::BinOp ) {
        auto* binOp = static_cast<BinOpNode*>(node);
        genExpr(binOp->left.get());  // Load left operand into rax
        out << "    push rax\n";      // Save left operand on stack
        genExpr(binOp->right.get()); // Load right operand into rax
        out << "    mov rbx, rax\n";   // Move right operand to rbx
        out << "    pop rax\n";       // Retrieve left operand from stack into rax

        if ( binOp->op == "+" ) {
            out << "    add rax, rbx\n"; // rax = left + right
        } else if ( binOp->op == "-" ) {
            out << "    sub rax, rbx\n"; // rax = left - right
        } else if ( binOp->op == "*" ) {
            out << "    imul rax, rbx\n"; // rax = left * right
        } else if ( binOp->op == "/" ) {
            int id = labelCounter++;
            
            // Check for division by zero
            // if yes print error message and exit
            // else perform the division ( can do signed and unsigned division )
            out << "    cmp rbx, 0\n";
            out << "    jne .Ldivsafe" << id << "\n";
            out << "    mov rax, 1\n";
            out << "    mov rdi, 1\n";
            out << "    lea rsi, divzero_msg\n";
            out << "    mov rdx, 17\n";
            out << "    syscall\n";
            out << "    mov rax, 60\n";
            out << "    mov rdi, 1\n";
            out << "    syscall\n";
            out << ".Ldivsafe" << id << ":\n";
            out << "    cqo\n";
            out << "    idiv rbx\n";    
        } else {
            throw std::runtime_error("AsmGen: unsupported binary operator '" + binOp->op + "'");
        }
    } else {
        throw std::runtime_error("AsmGen: unsupported node type for loading value into rax.");
    }
}

void AsmGen::genPrintRuntime() {
    int id = labelCounter++;
    out << "    lea r8, [printBuf + 31]\n";
    out << "    mov byte ptr [r8], 10\n";      // '\n'
    out << "    dec r8\n";

    out << "    xor r9, r9\n";                  // r9 = 0 means "was not negative"
    out << "    cmp rax, 0\n";
    out << "    jge .Lnonneg" << id << "\n";    // signed comparison: jump if rax >= 0
    out << "    neg rax\n";                     // flip to positive (two's complement negation)
    out << "    mov r9, 1\n";                   // remember: this one was negative
    out << ".Lnonneg" << id << ":\n";

    out << "    cmp rax, 0\n";
    out << "    jne .Lloop" << id << "\n";
    out << "    mov byte ptr [r8], 48\n";
    out << "    dec r8\n";
    out << "    jmp .Lsign" << id << "\n";

    out << ".Lloop" << id << ":\n";
    out << "    cmp rax, 0\n";
    out << "    je .Lsign" << id << "\n";
    out << "    xor rdx, rdx\n";
    out << "    mov rbx, 10\n";
    out << "    div rbx\n";
    out << "    add dl, 48\n";
    out << "    mov [r8], dl\n";
    out << "    dec r8\n";
    out << "    jmp .Lloop" << id << "\n";

    out << ".Lsign" << id << ":\n";
    out << "    cmp r9, 0\n";
    out << "    je .Ldone" << id << "\n";
    out << "    mov byte ptr [r8], 45\n";       // '-' is ASCII 45
    out << "    dec r8\n";

    out << ".Ldone" << id << ":\n";
    out << "    inc r8\n";
    out << "    lea rax, [printBuf + 32]\n";
    out << "    sub rax, r8\n";
    out << "    mov rdx, rax\n";
    out << "    mov rsi, r8\n";
    out << "    mov rdi, 1\n";
    out << "    mov rax, 1\n";
    out << "    syscall\n";

}