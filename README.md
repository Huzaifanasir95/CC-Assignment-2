
---

**National University of Computer and Emerging Sciences**  
**FAST School of Computing**  
**CS-4031: Compiler Construction**  
**Assignment #02 Report**  
**Spring 2025**  
**Submitted by:** Huzaifa Nasir 	22I-1053 	CS-A
Maaz Ali		22i-1042	CS-A  
**Date:** March 22, 2025  

---

### Introduction
This report outlines our approach to Assignment #02, which required designing and implementing a C program to process a Context-Free Grammar (CFG) by performing left factoring, left recursion removal, computing FIRST and FOLLOW sets, and constructing an LL(1) parsing table. The program reads a CFG from a file, processes it step-by-step, and outputs the results to a log file. This document details our methodology, challenges encountered, and the steps taken to verify the correctness of our implementation.

---

### Approach
Our implementation follows a modular design in C, adhering to the assignment’s requirements of generality, reusability, and no use of built-in compiler libraries. Below is an overview of our approach for each task:

1. **CFG Input Parsing**  
   - **Method**: We used a function (`readGrammarFromFile`) to read productions from `grammar.txt` in the format `NonTerminal->Production1|Production2`. Productions are stored in a `GrammarRule` struct, with non-terminals as single uppercase letters and productions as strings.
   - **Data Structure**: An array of `GrammarRule` structs, where each rule contains a head non-terminal and an array of production strings.

2. **Left Factoring**  
   - **Method**: The `leftFactorGrammar` function identifies common prefixes between pairs of productions for a non-terminal (e.g., `A->ab|ac` becomes `A->aX`, `X->b|c`). New non-terminals are introduced dynamically (e.g., `X`, `Y`).
   - **Algorithm**: Iteratively scan productions, extract prefixes using `commonPrefix`, and refactor until no changes occur.

3. **Left Recursion Removal**  
   - **Method**: The `removeLeftRecursion` function transforms left-recursive productions (e.g., `E->E+T|T` to `E->TE'`, `E'->+TE'|EPSILON`) using the standard algorithm. EPSILON is represented as `~`.
   - **Algorithm**: Separate recursive (`E->Eα`) and non-recursive (`β`) parts, introduce a new non-terminal (e.g., `E'`), and rewrite rules.

4. **FIRST Set Computation**  
   - **Method**: `computeFirstSets` and `computeFirst` recursively compute FIRST sets for each non-terminal, iterating until stable. For `X->Y1Y2`, FIRST(X) includes FIRST(Y1), continuing if Y1 derives ε.
   - **Data Structure**: Stored as strings in a `Sets` struct array.

5. **FOLLOW Set Computation**  
   - **Method**: `computeFollowSets` iteratively builds FOLLOW sets, starting with `$` for the start symbol. For `A->αBβ`, FIRST(β) (excluding ε) is added to FOLLOW(B), and FOLLOW(A) is propagated if β is nullable.
   - **Data Structure**: Stored alongside FIRST sets in `Sets`.

6. **LL(1) Parsing Table Construction**  
   - **Method**: `buildLL1Table` populates a 3D array (`LL1Table`) using FIRST and FOLLOW sets. For `A->α`, entries are set for each terminal in FIRST(α), and FOLLOW(A) if α derives ε.
   - **Data Structure**: A table indexed by non-terminal and terminal indices, with productions as strings.

7. **Output Generation**  
   - **Method**: Results are displayed after each step using `displayGrammar`, `displaySets`, and `displayLL1Table`, redirected to `output.txt`.

---

### Challenges Faced
1. **Left Factoring Complexity**  
   - **Issue**: Our initial approach only factored pairs of productions, potentially missing nested common prefixes (e.g., `A->abc|abd|abef`).  
   - **Solution**: We implemented an iterative process to handle multiple passes, though it’s still limited to pairwise factoring. A more sophisticated algorithm could fully factor in one pass.

2. **Non-Terminal Naming**  
   - **Issue**: Generating unique new non-terminals (e.g., `X`, `Z-i`) risked conflicts with existing grammar symbols.  
   - **Solution**: We used a simple offset scheme (`X+i`, `Z-i`), which worked for test cases but isn’t foolproof. A suffix approach (e.g., `E'`) would be more robust.

3. **Fixed Array Sizes**  
   - **Issue**: The use of fixed-size arrays (`MAX_NONTERMS`, `MAX_PRODS`) limits scalability for large grammars.  
   - **Solution**: For this assignment’s scope, the sizes sufficed, but dynamic allocation would be necessary for production use.

4. **Terminal Handling**  
   - **Issue**: The code assumes single-character terminals, splitting multi-character terminals (e.g., `ab` into `a` and `b`).  
   - **Solution**: We retained this for simplicity, as the example CFG used single characters. A tokenization layer could address this in future iterations.

---

### Verification of Correctness
To ensure our program works correctly, we tested it with multiple CFGs, comparing outputs to manual calculations. Below are the test cases and results:

1. **Simple CFG (Provided Example)**  
   - **Input**: `S->aB|c`, `B->b`  
   - **Output**: Correctly showed no factoring/recursion, `FIRST(S)={a,c}`, `FOLLOW(B)={$}`, and a valid LL(1) table.  
   - **Verification**: Matched hand-computed sets and table.

2. **Left Factoring CFG**  
   - **Input**: `A->ab|ac|d`, `B->b`  
   - **Output**: Factored to `A->aX|d`, `X->b|c`, with correct sets (`FIRST(A)={a,d}`, `FIRST(X)={b,c}`) and table.  
   - **Verification**: Confirmed factoring preserved the language and table was LL(1).

3. **Left Recursion CFG**  
   - **Input**: `E->E+T|T`, `T->t`  
   - **Output**: Removed recursion to `E->TE'`, `E'->+TE'|~`, with `FIRST(E)={t}`, `FOLLOW(T)={+,$}`, and a conflict-free table.  
   - **Verification**: Validated against the assignment’s example transformation.

4. **Nullable CFG**  
   - **Input**: `S->AB`, `A->a|EPSILON`, `B->b|EPSILON`  
   - **Output**: Correct FIRST (`S={a,b,~}`) and FOLLOW sets, but the table showed conflicts (not LL(1)), as expected.  
   - **Verification**: Confirmed nullable handling, noting the grammar’s non-LL(1) nature.

We manually traced each step for these CFGs, ensuring transformations preserved the language and sets/table followed standard algorithms. The code’s output log (`output.txt`) was cross-checked with these calculations.

---

### Conclusion
Our implementation successfully meets the assignment’s requirements, processing generic CFGs through all specified steps and producing the required outputs. While challenges like limited factoring and fixed sizes were encountered, they were mitigated within the assignment’s scope. Verification through diverse test cases confirmed correctness for typical LL(1) grammars. Future improvements could include dynamic memory, multi-character terminal support, and conflict detection in the LL(1) table.

