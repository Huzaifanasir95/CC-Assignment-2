#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NONTERMS  20
#define MAX_PRODS     20
#define MAX_SYMBOLS   20
#define MAX_CHAR      100
#define EPSILON '~'

typedef struct {
    char head;
    int  prodCount;
    char productions[MAX_PRODS][MAX_SYMBOLS]; 
} GrammarRule;

typedef struct {
    char nonTerm;
    char firstSet[MAX_SYMBOLS];
    char followSet[MAX_SYMBOLS];
} Sets;

GrammarRule grammar[MAX_NONTERMS];
Sets       setsTable[MAX_NONTERMS];
int        grammarCount = 0;

char LL1Table[MAX_NONTERMS][MAX_SYMBOLS][MAX_SYMBOLS];

int  findNonTermIndex(char head);
void initSetsTable(void);
void addToSet(char *set, char symbol);
int  containsSymbol(const char *set, char symbol);
int  isNonTerminal(char c);
int  isTerminal(char c);

void readGrammarFromFile(const char *filename);
void displayGrammar(const char *msg);

void leftFactorGrammar();
void removeLeftRecursion();

void computeFirstSets();
void computeFollowSets();

void buildLL1Table();

void displaySets();
void displayLL1Table();

int main() {
    FILE *f = fopen("output.txt", "w");
    if(!f) {
        printf("Could not open output.txt\n");
        return 1;
    }
    freopen("output.txt", "w", stdout);
    readGrammarFromFile("grammar.txt");
    displayGrammar("Initial Grammar:");

    leftFactorGrammar();
    displayGrammar("After Left Factoring:");

    removeLeftRecursion();
    displayGrammar("After Removing Left Recursion:");

    initSetsTable();
    computeFirstSets();

    computeFollowSets();

    displaySets();

    buildLL1Table();

    displayLL1Table();
    fclose(f);
    return 0;
}

int findNonTermIndex(char head) {
    for(int i = 0; i < grammarCount; i++) {
        if(grammar[i].head == head) {
            return i;
        }
    }
    return -1;
}

void initSetsTable(void) {
    for(int i = 0; i < grammarCount; i++) {
        setsTable[i].nonTerm = grammar[i].head;
        setsTable[i].firstSet[0] = '\0';
        setsTable[i].followSet[0] = '\0';
    }
}

void addToSet(char *set, char symbol) {
    if (!containsSymbol(set, symbol)) {
        int len = strlen(set);
        set[len] = symbol;
        set[len+1] = '\0';
    }
}

int containsSymbol(const char *set, char symbol) {
    return (strchr(set, symbol) != NULL);
}

int isNonTerminal(char c) {
    return (c >= 'A' && c <= 'Z');
}

int isTerminal(char c) {
    return (!isNonTerminal(c) && c != EPSILON && c != '\0');
}

void readGrammarFromFile(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if(!fp) {
        printf("Error: Could not open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_CHAR];
    while(fgets(buffer, MAX_CHAR, fp)) {
        buffer[strcspn(buffer, "\r\n")] = '\0';

        if(strlen(buffer) < 3) continue;

        char *arrowPos = strstr(buffer, "->");
        if(!arrowPos) continue;

        char head = buffer[0];
        int idx = findNonTermIndex(head);

        if(idx == -1) {
            idx = grammarCount++;
            grammar[idx].head = head;
            grammar[idx].prodCount = 0;
        }

        char *rhs = arrowPos + 2;

        char *token = strtok(rhs, "|");
        while(token) {
            strcpy(grammar[idx].productions[grammar[idx].prodCount++], token);
            token = strtok(NULL, "|");
        }
    }
    fclose(fp);
}

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

static void commonPrefix(const char *s1, const char *s2, char *prefix) {
    int i = 0;
    while(s1[i] && s2[i] && s1[i] == s2[i]) {
        prefix[i] = s1[i];
        i++;
    }
    prefix[i] = '\0';
}

void leftFactorGrammar() {
    int changed = 1;
    while(changed) {
        changed = 0;
        for(int i = 0; i < grammarCount; i++) {
            for(int p1 = 0; p1 < grammar[i].prodCount; p1++) {
                for(int p2 = p1 + 1; p2 < grammar[i].prodCount; p2++) {
                    char prefix[MAX_SYMBOLS];
                    commonPrefix(grammar[i].productions[p1], grammar[i].productions[p2], prefix);

                    if(strlen(prefix) > 0) {
                        char newHead = (char)('X' + i);
                        if(findNonTermIndex(newHead) == -1 && isNonTerminal(newHead)) {
                            int newIdx = grammarCount++;
                            grammar[newIdx].head = newHead;
                            grammar[newIdx].prodCount = 0;

                            char leftover1[MAX_SYMBOLS];
                            char leftover2[MAX_SYMBOLS];

                            strcpy(leftover1, grammar[i].productions[p1] + strlen(prefix));
                            strcpy(leftover2, grammar[i].productions[p2] + strlen(prefix));

                            if(strlen(leftover1) == 0) strcpy(leftover1, "EPSILON");
                            if(strlen(leftover2) == 0) strcpy(leftover2, "EPSILON");

                            strcpy(grammar[newIdx].productions[grammar[newIdx].prodCount++], leftover1);
                            strcpy(grammar[newIdx].productions[grammar[newIdx].prodCount++], leftover2);

                            sprintf(grammar[i].productions[p1], "%s%c", prefix, newHead);

                            for(int k = p2; k < grammar[i].prodCount - 1; k++) {
                                strcpy(grammar[i].productions[k], grammar[i].productions[k+1]);
                            }
                            grammar[i].prodCount--;

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

void removeLeftRecursion() {
    for(int i = 0; i < grammarCount; i++) {
        char A = grammar[i].head;

        int hasLeftRecursion = 0;
        char alpha[MAX_PRODS][MAX_SYMBOLS];
        char beta[MAX_PRODS][MAX_SYMBOLS];
        int alphaCount = 0;
        int betaCount = 0;

        for(int j = 0; j < grammar[i].prodCount; j++) {
            const char *prod = grammar[i].productions[j];
            if(prod[0] == A) {
                hasLeftRecursion = 1;
                strcpy(alpha[alphaCount++], prod + 1);
            } else {
                strcpy(beta[betaCount++], prod);
            }
        }

        if(hasLeftRecursion) {
            char Aprime = (char)('Z' - i);
            if(!isNonTerminal(Aprime) || findNonTermIndex(Aprime) != -1) {
            }
            int newIndex = findNonTermIndex(Aprime);
            if(newIndex == -1) {
                newIndex = grammarCount++;
                grammar[newIndex].head = Aprime;
                grammar[newIndex].prodCount = 0;
            }

            grammar[i].prodCount = 0;
            for(int b = 0; b < betaCount; b++) {
                char newProd[MAX_SYMBOLS];
                sprintf(newProd, "%s%c", beta[b], Aprime);
                strcpy(grammar[i].productions[grammar[i].prodCount++], newProd);
            }

            if(betaCount == 0) {
                char newProd[MAX_SYMBOLS];
                sprintf(newProd, "EPSILON%c", Aprime);
                strcpy(grammar[i].productions[grammar[i].prodCount++], newProd);
            }

            grammar[newIndex].prodCount = 0;
            for(int a = 0; a < alphaCount; a++) {
                char newProd[MAX_SYMBOLS];
                sprintf(newProd, "%s%c", alpha[a], Aprime);
                strcpy(grammar[newIndex].productions[grammar[newIndex].prodCount++], newProd);
            }
            strcpy(grammar[newIndex].productions[grammar[newIndex].prodCount++], "EPSILON");
        }
    }
}

void computeFirst(char X, char *result);

void computeFirstSets() {
    int done = 0;
    while(!done) {
        done = 1;
        for(int i = 0; i < grammarCount; i++) {
            char currentFirst[MAX_SYMBOLS];
            strcpy(currentFirst, setsTable[i].firstSet);

            computeFirst(grammar[i].head, setsTable[i].firstSet);

            if(strcmp(currentFirst, setsTable[i].firstSet) != 0) {
                done = 0;
            }
        }
    }
}

void computeFirst(char X, char *result) {
    if(isTerminal(X) || X == EPSILON) {
        addToSet(result, X);
        return;
    }

    int idx = findNonTermIndex(X);
    if(idx == -1) return;

    for(int p = 0; p < grammar[idx].prodCount; p++) {
        const char *prod = grammar[idx].productions[p];

        int k = 0;
        while(prod[k] != '\0') {
            char subFirst[MAX_SYMBOLS] = "";
            computeFirst(prod[k], subFirst);

            for(int m = 0; m < (int)strlen(subFirst); m++) {
                if(subFirst[m] != EPSILON) {
                    addToSet(result, subFirst[m]);
                }
            }

            if(!containsSymbol(subFirst, EPSILON)) {
                break;
            }

            k++;
        }

        if(prod[k] == '\0') {
            addToSet(result, EPSILON);
        }
    }
}

void computeFollowSets() {
    if(grammarCount > 0) {
        addToSet(setsTable[0].followSet, '$');
    }

    int changed = 1;
    while(changed) {
        changed = 0;
        for(int i = 0; i < grammarCount; i++) {
            char A = grammar[i].head;
            char *followA = NULL;
            for(int sf = 0; sf < grammarCount; sf++) {
                if(setsTable[sf].nonTerm == A) {
                    followA = setsTable[sf].followSet;
                    break;
                }
            }

            for(int p = 0; p < grammar[i].prodCount; p++) {
                const char *prod = grammar[i].productions[p];
                int lenProd = strlen(prod);

                for(int pos = 0; pos < lenProd; pos++) {
                    char B = prod[pos];
                    if(isNonTerminal(B)) {
                        char *followB = NULL;
                        for(int sf = 0; sf < grammarCount; sf++) {
                            if(setsTable[sf].nonTerm == B) {
                                followB = setsTable[sf].followSet;
                                break;
                            }
                        }
                        if(!followB) continue;

                        char betaFirst[MAX_SYMBOLS] = "";
                        int q = pos + 1;
                        int allEpsilon = 1;
                        while(q < lenProd) {
                            char subFirst[MAX_SYMBOLS] = "";
                            computeFirst(prod[q], subFirst);

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

                        if(q == lenProd && allEpsilon) {
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

static char terminals[MAX_SYMBOLS];
static int  terminalCount = 0;

static void gatherTerminals() {
    terminalCount = 0;
    memset(terminals, 0, sizeof(terminals));

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

    if(!containsSymbol(terminals, '$')) {
        int len = strlen(terminals);
        terminals[len] = '$';
        terminals[len+1] = '\0';
        terminalCount++;
    }
}

static int getTerminalIndex(char c) {
    char *pos = strchr(terminals, c);
    if(pos) return (int)(pos - terminals);
    return -1;
}

void buildLL1Table() {
    gatherTerminals();

    for(int i = 0; i < MAX_NONTERMS; i++) {
        for(int j = 0; j < MAX_SYMBOLS; j++) {
            LL1Table[i][j][0] = '\0';
        }
    }

    for(int i = 0; i < grammarCount; i++) {
        char A = grammar[i].head;
        int Arow = i;

        for(int p = 0; p < grammar[i].prodCount; p++) {
            char prod[MAX_SYMBOLS];
            strcpy(prod, grammar[i].productions[p]);

            char firstP[MAX_SYMBOLS] = "";
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

            if(containsSymbol(firstP, EPSILON)) {
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

void displayLL1Table() {
    printf("\nLL(1) Parsing Table:\n");

    printf("        ");
    for(int c = 0; c < terminalCount; c++) {
        printf("  %c   ", terminals[c]);
    }
    printf("\n");

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