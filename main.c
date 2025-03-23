#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NONTERMS  20   // Max number of distinct nonterminals
#define MAX_PRODS     20   // Max number of productions per nonterminal
#define MAX_SYMBOLS   20   // Max length of a single production
#define MAX_CHAR      100  // Max length of a line in the grammar file
#define EPSILON '~'

// --------------------------------------------------
// Data Structures
// --------------------------------------------------

/*
 * Each GrammarRule holds:
 *   head: The nonterminal (e.g. 'E')
 *   prodCount: Number of productions for that nonterminal
 *   productions: Each production (RHS) stored as a string, e.g. "E+T" or "T"
 */
typedef struct {
    char head;                   // Single-character nonterminal
    int  prodCount;             // Number of productions
    char productions[MAX_PRODS][MAX_SYMBOLS]; 
} GrammarRule;

/*
 * For storing FIRST and FOLLOW sets:
 *   nonTerm: The nonterminal (e.g. 'E')
 *   firstSet: Characters in FIRST(E)
 *   followSet: Characters in FOLLOW(E)
 */
typedef struct {
    char nonTerm;
    char firstSet[MAX_SYMBOLS];
    char followSet[MAX_SYMBOLS];
} Sets;

/*
 * Global arrays:
 *   grammar[]: Stores the grammar
 *   setsTable[]: Stores FIRST and FOLLOW sets
 *   grammarCount: Number of distinct nonterminals
 */
GrammarRule grammar[MAX_NONTERMS];
Sets       setsTable[MAX_NONTERMS];
int        grammarCount = 0;

// The LL(1) parsing table, stored as strings for each (nonterminal, terminal).
// For example, LL1Table[row][col] might store "E->E+T" or "E->T" to indicate
// which production is used. Empty string = no entry.
char LL1Table[MAX_NONTERMS][MAX_SYMBOLS][MAX_SYMBOLS];

// --------------------------------------------------
// Function Prototypes
// --------------------------------------------------

// Utility
int  findNonTermIndex(char head);
void initSetsTable(void);
void addToSet(char *set, char symbol);
int  containsSymbol(const char *set, char symbol);
int  isNonTerminal(char c);
int  isTerminal(char c);

// Steps
void readGrammarFromFile(const char *filename);
void displayGrammar(const char *msg);

void leftFactorGrammar();
void removeLeftRecursion();

void computeFirstSets();
void computeFollowSets();

void buildLL1Table();

void displaySets();
void displayLL1Table();

// --------------------------------------------------
// Main
// --------------------------------------------------

int main() {
    FILE *f = fopen("output.txt", "w");
    if(!f) {
        printf("Could not open output.txt\n");
        return 1;
    }
    // Redirect stdout to our file
    freopen("output.txt", "w", stdout);
    // 1. Read grammar from a file (assuming "grammar.txt" in the same directory)
    readGrammarFromFile("grammar.txt");
    displayGrammar("Initial Grammar:");

    // 2. Left factoring
    leftFactorGrammar();
    displayGrammar("After Left Factoring:");

    // 3. Remove left recursion
    removeLeftRecursion();
    displayGrammar("After Removing Left Recursion:");

    // 4. Compute FIRST sets
    initSetsTable();
    computeFirstSets();

    // 5. Compute FOLLOW sets
    computeFollowSets();

    // Display FIRST and FOLLOW sets
    displaySets();

    // 6. Build LL(1) parsing table
    buildLL1Table();

    // 7. Display LL(1) parsing table
    displayLL1Table();
    fclose(f);
    return 0;
}

// --------------------------------------------------
// Utility Functions
// --------------------------------------------------

/*
 * findNonTermIndex: Return the index of 'head' in grammar[].
 *                   Return -1 if not found.
 */
int findNonTermIndex(char head) {
    for(int i = 0; i < grammarCount; i++) {
        if(grammar[i].head == head) {
            return i;
        }
    }
    return -1;
}

/*
 * initSetsTable: Initialize the setsTable to hold empty FIRST and FOLLOW
 *                sets for each grammar nonterminal.
 */
void initSetsTable(void) {
    for(int i = 0; i < grammarCount; i++) {
        setsTable[i].nonTerm = grammar[i].head;
        setsTable[i].firstSet[0] = '\0';
        setsTable[i].followSet[0] = '\0';
    }
}

/*
 * addToSet: Add 'symbol' to the given set string if not already present.
 */
void addToSet(char *set, char symbol) {
    if (!containsSymbol(set, symbol)) {
        int len = strlen(set);
        set[len] = symbol;
        set[len+1] = '\0';
    }
}

/*
 * containsSymbol: Returns 1 if set has symbol, otherwise 0.
 */
int containsSymbol(const char *set, char symbol) {
    return (strchr(set, symbol) != NULL);
}

/*
 * isNonTerminal: Returns 1 if 'c' is uppercase letter (A-Z).
 *                Adjust if your grammar uses a different convention.
 */
int isNonTerminal(char c) {
    return (c >= 'A' && c <= 'Z');
}

/*
 * isTerminal: Returns 1 if 'c' is NOT an uppercase letter and not the empty symbol.
 *             Adjust to your grammar as needed.
 */
int isTerminal(char c) {
    return (!isNonTerminal(c) && c != EPSILON && c != '\0');
}

// --------------------------------------------------
// Step 1: Read Grammar
// --------------------------------------------------

/*
 * Expects lines like:
 *    E->E+T|T
 *    T->T*F|F
 *    F->(E)|id
 */
void readGrammarFromFile(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if(!fp) {
        printf("Error: Could not open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_CHAR];
    while(fgets(buffer, MAX_CHAR, fp)) {
        // Remove newline
        buffer[strcspn(buffer, "\r\n")] = '\0';

        // Skip empty lines
        if(strlen(buffer) < 3) continue;

        // Find "->"
        char *arrowPos = strstr(buffer, "->");
        if(!arrowPos) continue;

        // The head nonterminal is the character before "->"
        char head = buffer[0];
        int idx = findNonTermIndex(head);

        // If new nonterminal, create new entry
        if(idx == -1) {
            idx = grammarCount++;
            grammar[idx].head = head;
            grammar[idx].prodCount = 0;
        }

        // Extract the RHS after "->"
        char *rhs = arrowPos + 2;

        // Split on '|'
        char *token = strtok(rhs, "|");
        while(token) {
            strcpy(grammar[idx].productions[grammar[idx].prodCount++], token);
            token = strtok(NULL, "|");
        }
    }
    fclose(fp);
}

/*
 * Display the grammar productions to stdout.
 */
void displayGrammar(const char *msg) {
    printf("\n%s\n", msg);
    for(int i = 0; i < grammarCount; i++) {
        printf("%c -> ", grammar[i].head);
        for(int j = 0; j < grammar[i].prodCount; j++) {
            printf("%s", grammar[i].productions[j]);
            if(j < grammar[i].prodCount - 1) printf(" | ");
        }
        printf("\n");
    }
}

// --------------------------------------------------
// Step 2: Left Factoring
// --------------------------------------------------

/*
 * A helper that finds the longest common prefix among two strings (productions).
 * Example: commonPrefix("abc", "abd") = "ab".
 */
static void commonPrefix(const char *s1, const char *s2, char *prefix) {
    int i = 0;
    while(s1[i] && s2[i] && s1[i] == s2[i]) {
        prefix[i] = s1[i];
        i++;
    }
    prefix[i] = '\0';
}

/*
 * For each nonterminal, check if multiple productions share a prefix.
 * If they do, factor it out into a new nonterminal. Repeat until no change.
 *
 * This is a *simplistic* approach that factors out the *largest* common prefix
 * of any pair of productions. Real grammars might need more careful iteration.
 */
void leftFactorGrammar() {
    int changed = 1;
    while(changed) {
        changed = 0;
        for(int i = 0; i < grammarCount; i++) {
            // Check each pair of productions for a common prefix
            for(int p1 = 0; p1 < grammar[i].prodCount; p1++) {
                for(int p2 = p1 + 1; p2 < grammar[i].prodCount; p2++) {
                    // Find prefix
                    char prefix[MAX_SYMBOLS];
                    commonPrefix(grammar[i].productions[p1], grammar[i].productions[p2], prefix);

                    // If prefix is non-empty and not equal to the entire production
                    if(strlen(prefix) > 0) {
                        // We'll factor out that prefix
                        // 1) Create new nonterminal, e.g. X'
                        //    We'll pick a letter not in the grammar. For simplicity, pick next capital letter.
                        //    In real usage, ensure uniqueness in a more robust way.
                        char newHead = (char)('X' + i); // simplistic attempt
                        if(findNonTermIndex(newHead) == -1 && isNonTerminal(newHead)) {
                            // Add new rule
                            int newIdx = grammarCount++;
                            grammar[newIdx].head = newHead;
                            grammar[newIdx].prodCount = 0;

                            // Factor out the prefix from both productions
                            // e.g. If prefix="ab", and the production is "abc", leftover is "c"
                            // If leftover is empty, we might insert EPSILON for epsilon
                            char leftover1[MAX_SYMBOLS];
                            char leftover2[MAX_SYMBOLS];

                            strcpy(leftover1, grammar[i].productions[p1] + strlen(prefix));
                            strcpy(leftover2, grammar[i].productions[p2] + strlen(prefix));

                            // If leftover is "", that means the entire production is the prefix => use EPSILON
                            if(strlen(leftover1) == 0) strcpy(leftover1, "EPSILON");
                            if(strlen(leftover2) == 0) strcpy(leftover2, "EPSILON");

                            // Add these as new productions
                            strcpy(grammar[newIdx].productions[grammar[newIdx].prodCount++], leftover1);
                            strcpy(grammar[newIdx].productions[grammar[newIdx].prodCount++], leftover2);

                            // Now rewrite the original productions in grammar[i]
                            // We'll keep only the prefix + newHead in the first production,
                            // and remove the second production (p2).
                            // e.g. A->abc | abd becomes A->abX' and X'->c|d
                            // We'll place them at p1, remove p2, shift the rest up
                            sprintf(grammar[i].productions[p1], "%s%c", prefix, newHead);

                            // Remove p2 by shifting all subsequent productions
                            for(int k = p2; k < grammar[i].prodCount - 1; k++) {
                                strcpy(grammar[i].productions[k], grammar[i].productions[k+1]);
                            }
                            grammar[i].prodCount--;

                            // Mark changed, break out to restart scanning
                            changed = 1;
                            goto restartFactor;
                        }
                    }
                }
            }
        }
    restartFactor:;
    }
}

// --------------------------------------------------
// Step 3: Remove Left Recursion
// --------------------------------------------------

/*
 * Remove left recursion from each nonterminal using the standard approach:
 *   If A -> A α1 | A α2 | ... | β1 | β2 ...
 *   Then rewrite:
 *       A -> β1 A' | β2 A' | ...
 *       A' -> α1 A' | α2 A' | ... | EPSILON
 */
void removeLeftRecursion() {
    for(int i = 0; i < grammarCount; i++) {
        char A = grammar[i].head;

        // Find all productions that begin with A (left-recursive)
        int hasLeftRecursion = 0;
        char alpha[MAX_PRODS][MAX_SYMBOLS];
        char beta[MAX_PRODS][MAX_SYMBOLS];
        int alphaCount = 0;
        int betaCount = 0;

        // Separate left-recursive vs non-left-recursive
        for(int j = 0; j < grammar[i].prodCount; j++) {
            const char *prod = grammar[i].productions[j];
            if(prod[0] == A) {
                // A -> A X ...
                hasLeftRecursion = 1;
                // Store the part after A
                strcpy(alpha[alphaCount++], prod + 1);
            } else {
                // A -> something else
                strcpy(beta[betaCount++], prod);
            }
        }

        if(hasLeftRecursion) {
            // We have left recursion for A
            // Create a new nonterminal A'
            char Aprime = (char)('Z' - i); // a quick attempt to get a letter
            if(!isNonTerminal(Aprime) || findNonTermIndex(Aprime) != -1) {
                // If that's not good, pick something else. For simplicity, assume success here.
            }
            int newIndex = findNonTermIndex(Aprime);
            if(newIndex == -1) {
                newIndex = grammarCount++;
                grammar[newIndex].head = Aprime;
                grammar[newIndex].prodCount = 0;
            }

            // For the original A:
            // A -> beta1 A' | beta2 A' | ... if any betas exist
            grammar[i].prodCount = 0;
            for(int b = 0; b < betaCount; b++) {
                // A -> beta[b] A'
                char newProd[MAX_SYMBOLS];
                sprintf(newProd, "%s%c", beta[b], Aprime);
                strcpy(grammar[i].productions[grammar[i].prodCount++], newProd);
            }

            // If no beta was found, we do A -> EPSILON A'
            if(betaCount == 0) {
                char newProd[MAX_SYMBOLS];
                sprintf(newProd, "EPSILON%c", Aprime);
                strcpy(grammar[i].productions[grammar[i].prodCount++], newProd);
            }

            // For A': A' -> alpha1 A' | alpha2 A' | ... | EPSILON
            grammar[newIndex].prodCount = 0;
            for(int a = 0; a < alphaCount; a++) {
                // A' -> alpha[a] A'
                char newProd[MAX_SYMBOLS];
                sprintf(newProd, "%s%c", alpha[a] + 1, Aprime); 
                // +1 because alpha[a] might be something like "abc" and we skip the first char if it was A
                // But in this code, alpha[a] already excludes the left recursion symbol, so be mindful.
                // If alpha[a] was the entire leftover, we might not skip anything. Adjust as needed.
                // For example, if alpha[a] = "XYZ", we do "XYZ A'"
                // Here we do an offset if the first char was empty space, etc.
                // For simplicity, let's assume alpha[a] is the portion after the nonterminal, so we can do:
                // sprintf(newProd, "%s%c", alpha[a], Aprime);

                sprintf(newProd, "%s%c", alpha[a], Aprime);
                strcpy(grammar[newIndex].productions[grammar[newIndex].prodCount++], newProd);
            }
            // A' -> EPSILON
            strcpy(grammar[newIndex].productions[grammar[newIndex].prodCount++], "EPSILON");
        }
    }
}

// --------------------------------------------------
// Step 4: Compute FIRST Sets
// --------------------------------------------------

/*
 * forward declaration for recursion
 */
void computeFirst(char X, char *result);

/*
 * If X is terminal, FIRST(X) = { X }.
 * If X is nonterminal, FIRST(X) is union of FIRST of each production.
 *   For production X->Y1Y2...
 *      add FIRST(Y1) minus EPSILON
 *      if EPSILON in FIRST(Y1), add FIRST(Y2) minus EPSILON
 *      etc.
 */
void computeFirstSets() {
    // We'll repeatedly call computeFirst on each nonterminal until stable
    // For simplicity, we do a few passes.
    int done = 0;
    while(!done) {
        done = 1;
        for(int i = 0; i < grammarCount; i++) {
            char currentFirst[MAX_SYMBOLS];
            strcpy(currentFirst, setsTable[i].firstSet);

            // Compute FIRST of that nonterminal, store results in setsTable[i].firstSet
            computeFirst(grammar[i].head, setsTable[i].firstSet);

            // If it changed, we keep going
            if(strcmp(currentFirst, setsTable[i].firstSet) != 0) {
                done = 0;
            }
        }
    }
}

/*
 * computeFirst: add FIRST(X) to 'result' set (i.e., union).
 *               'result' must be a null-terminated string initially.
 */
void computeFirst(char X, char *result) {
    // If X is terminal or EPSILON
    if(isTerminal(X) || X == EPSILON) {
        addToSet(result, X);
        return;
    }

    // Otherwise, X is a nonterminal
    int idx = findNonTermIndex(X);
    if(idx == -1) return; // not found, skip

    // For each production X->Y1Y2...
    for(int p = 0; p < grammar[idx].prodCount; p++) {
        const char *prod = grammar[idx].productions[p];

        // For each symbol in this production
        int k = 0;
        while(prod[k] != '\0') {
            // Compute FIRST(Yk)
            char subFirst[MAX_SYMBOLS] = "";
            computeFirst(prod[k], subFirst);

            // Add all from subFirst except EPSILON to result
            for(int m = 0; m < (int)strlen(subFirst); m++) {
                if(subFirst[m] != EPSILON) {
                    addToSet(result, subFirst[m]);
                }
            }

            // If subFirst contains EPSILON, we continue to next symbol
            // else, we stop
            if(!containsSymbol(subFirst, EPSILON)) {
                break;
            }

            k++;
        }

        // If we consumed all symbols and all contained EPSILON,
        // then add EPSILON to FIRST(X)
        if(prod[k] == '\0') {
            addToSet(result, EPSILON);
        }
    }
}

// --------------------------------------------------
// Step 5: Compute FOLLOW Sets
// --------------------------------------------------

/*
 * We'll implement a standard iterative approach:
 *   1) Place $ in FOLLOW(S) where S is the start symbol.
 *   2) If A -> αBβ, then everything in FIRST(β) except EPSILON is placed in FOLLOW(B).
 *   3) If A -> αB or A -> αBβ where EPSILON in FIRST(β), then everything in FOLLOW(A)
 *      is in FOLLOW(B).
 * Repeat until stable.
 */

void computeFollowSets() {
    // Step 1: add '$' to FOLLOW(startSymbol)
    if(grammarCount > 0) {
        addToSet(setsTable[0].followSet, '$');
    }

    int changed = 1;
    while(changed) {
        changed = 0;
        for(int i = 0; i < grammarCount; i++) {
            char A = grammar[i].head;
            // get FOLLOW(A)
            char *followA = NULL;
            for(int sf = 0; sf < grammarCount; sf++) {
                if(setsTable[sf].nonTerm == A) {
                    followA = setsTable[sf].followSet;
                    break;
                }
            }

            // For each production A->α
            for(int p = 0; p < grammar[i].prodCount; p++) {
                const char *prod = grammar[i].productions[p];
                int lenProd = strlen(prod);

                for(int pos = 0; pos < lenProd; pos++) {
                    char B = prod[pos];
                    if(isNonTerminal(B)) {
                        // Find FOLLOW(B)
                        char *followB = NULL;
                        for(int sf = 0; sf < grammarCount; sf++) {
                            if(setsTable[sf].nonTerm == B) {
                                followB = setsTable[sf].followSet;
                                break;
                            }
                        }
                        if(!followB) continue;

                        // Everything in FIRST(β) except EPSILON goes to FOLLOW(B)
                        // β = prod[pos+1 .. end]
                        char betaFirst[MAX_SYMBOLS] = "";
                        int q = pos + 1;
                        int allEpsilon = 1;
                        while(q < lenProd) {
                            char subFirst[MAX_SYMBOLS] = "";
                            computeFirst(prod[q], subFirst);

                            // add all except EPSILON
                            for(int z = 0; z < (int)strlen(subFirst); z++) {
                                if(subFirst[z] != EPSILON) {
                                    if(!containsSymbol(followB, subFirst[z])) {
                                        addToSet(followB, subFirst[z]);
                                        changed = 1;
                                    }
                                }
                            }

                            if(!containsSymbol(subFirst, EPSILON)) {
                                allEpsilon = 0;
                                break;
                            }
                            q++;
                        }

                        // If all symbols from pos+1..end can derive EPSILON,
                        // then everything in FOLLOW(A) goes to FOLLOW(B).
                        if(q == lenProd && allEpsilon) {
                            // add followA to followB
                            for(int z = 0; z < (int)strlen(followA); z++) {
                                char sym = followA[z];
                                if(!containsSymbol(followB, sym)) {
                                    addToSet(followB, sym);
                                    changed = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// --------------------------------------------------
// Step 6: Build LL(1) Table
// --------------------------------------------------

/*
 * For each production A->α:
 *    For each terminal 'a' in FIRST(α): Table[A,a] = A->α
 *    If EPSILON in FIRST(α), then for each 'b' in FOLLOW(A): Table[A,b] = A->α
 *
 * We'll store the production in LL1Table[row][col] as a string "A->α".
 * row = index of A, col = index of terminal.
 * We'll do a simple approach: We'll store terminals in an array "terminals" and
 * map them to columns. This means you must gather all terminals from the grammar.
 */
static char terminals[MAX_SYMBOLS];
static int  terminalCount = 0;

/*
 * Gather terminals from the grammar and store them in 'terminals[]'.
 */
static void gatherTerminals() {
    // Reset
    terminalCount = 0;
    memset(terminals, 0, sizeof(terminals));

    // Naive approach: scan all productions, pick out single-char symbols that are not uppercase and not EPSILON.
    // Also add '$'.
    for(int i = 0; i < grammarCount; i++) {
        for(int j = 0; j < grammar[i].prodCount; j++) {
            const char *p = grammar[i].productions[j];
            for(int k = 0; k < (int)strlen(p); k++) {
                char c = p[k];
                if(isTerminal(c) && c != EPSILON) {
                    if(!containsSymbol(terminals, c)) {
                        int len = strlen(terminals);
                        terminals[len] = c;
                        terminals[len+1] = '\0';
                        terminalCount++;
                    }
                }
            }
        }
    }

    // Add '$' as well
    if(!containsSymbol(terminals, '$')) {
        int len = strlen(terminals);
        terminals[len] = '$';
        terminals[len+1] = '\0';
        terminalCount++;
    }
}

/*
 * Return the index of terminal c in 'terminals[]', or -1 if not found.
 */
static int getTerminalIndex(char c) {
    char *pos = strchr(terminals, c);
    if(pos) return (int)(pos - terminals);
    return -1;
}

/*
 * Build the LL(1) table: for each A->α, fill table entries based on FIRST(α) or FOLLOW(A) if α can derive EPSILON.
 */
void buildLL1Table() {
    // Gather terminals to define the columns
    gatherTerminals();

    // Initialize table: empty
    for(int i = 0; i < MAX_NONTERMS; i++) {
        for(int j = 0; j < MAX_SYMBOLS; j++) {
            LL1Table[i][j][0] = '\0';
        }
    }

    // For each production A->α
    for(int i = 0; i < grammarCount; i++) {
        char A = grammar[i].head;
        int Arow = i;  // we'll use the grammar index as the row

        // Find FIRST(α)
        for(int p = 0; p < grammar[i].prodCount; p++) {
            char prod[MAX_SYMBOLS];
            strcpy(prod, grammar[i].productions[p]);

            // Compute FIRST(prod)
            char firstP[MAX_SYMBOLS] = "";
            // We'll do a small function that merges FIRST sets of each symbol in prod
            {
                int k = 0;
                int allEpsilons = 1;
                while(prod[k] != '\0') {
                    char subF[MAX_SYMBOLS] = "";
                    computeFirst(prod[k], subF);
                    for(int s = 0; s < (int)strlen(subF); s++) {
                        if(subF[s] != EPSILON) {
                            addToSet(firstP, subF[s]);
                        }
                    }
                    if(!containsSymbol(subF, EPSILON)) {
                        allEpsilons = 0;
                        break;
                    }
                    k++;
                }
                if(allEpsilons) {
                    addToSet(firstP, EPSILON);
                }
            }

            // For each terminal 'a' in firstP (excluding EPSILON), set table[A,a] = "A->prod"
            char productionString[MAX_SYMBOLS];
            sprintf(productionString, "%c->%s", A, prod);

            for(int s = 0; s < (int)strlen(firstP); s++) {
                char terminal = firstP[s];
                if(terminal != EPSILON) {
                    int col = getTerminalIndex(terminal);
                    if(col >= 0) {
                        strcpy(LL1Table[Arow][col], productionString);
                    }
                }
            }

            // If EPSILON in firstP, for each b in FOLLOW(A), table[A,b] = "A->prod"
            if(containsSymbol(firstP, EPSILON)) {
                // find FOLLOW(A)
                char followA[MAX_SYMBOLS] = "";
                for(int sf = 0; sf < grammarCount; sf++) {
                    if(setsTable[sf].nonTerm == A) {
                        strcpy(followA, setsTable[sf].followSet);
                        break;
                    }
                }

                for(int s = 0; s < (int)strlen(followA); s++) {
                    int col = getTerminalIndex(followA[s]);
                    if(col >= 0) {
                        strcpy(LL1Table[Arow][col], productionString);
                    }
                }
            }
        }
    }
}

// --------------------------------------------------
// Step 7: Display Results
// --------------------------------------------------

/*
 * Display FIRST and FOLLOW sets for each nonterminal
 */
void displaySets() {
    printf("\nFIRST & FOLLOW Sets:\n");
    for(int i = 0; i < grammarCount; i++) {
        printf("  %c:\n", setsTable[i].nonTerm);
        printf("    FIRST  = { ");
        for(int j = 0; j < (int)strlen(setsTable[i].firstSet); j++) {
            printf("%c ", setsTable[i].firstSet[j]);
        }
        printf("}\n");

        printf("    FOLLOW = { ");
        for(int j = 0; j < (int)strlen(setsTable[i].followSet); j++) {
            printf("%c ", setsTable[i].followSet[j]);
        }
        printf("}\n\n");
    }
}

/*
 * Display the LL(1) parsing table. Rows are nonterminals, columns are terminals.
 */
void displayLL1Table() {
    printf("\nLL(1) Parsing Table:\n");

    // Print header row
    printf("        ");
    for(int c = 0; c < terminalCount; c++) {
        printf("  %c   ", terminals[c]);
    }
    printf("\n");

    // For each nonterminal row
    for(int r = 0; r < grammarCount; r++) {
        printf("  %c  | ", grammar[r].head);
        for(int c = 0; c < terminalCount; c++) {
            if(strlen(LL1Table[r][c]) > 0) {
                printf("%-6s", LL1Table[r][c]);
            } else {
                printf("  -   ");
            }
        }
        printf("\n");
    }
    printf("\n");
}
