#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRIPS 10
#define MAX_ACTIVITIES 100
#define MAX_NAV_STEPS 100
#define DATA_FILE "trips.dat"

typedef struct navNode {
    int stepId;
    char Direction[20];
    float Distance;
    int height;
    struct navNode* left;
    struct navNode* right;
} navNode;

typedef struct fNode {
    char From[50];
    char To[50];
} flight;

typedef struct hNode {
    char hotelName[50];
    int cost;
} hotel;

typedef struct tNode {
    char place[50];
} tourist;

typedef struct trNode {
    char mode[50];
    char from[50];
    char to[50];
} transport;

typedef union {
    flight Flight;
    hotel Hotel;
    tourist Tourist;
    transport Transport;
} ativity;

typedef struct tripNode {
    int activityId;
    int type;
    char date[15];
    float time;
    ativity data;
    navNode* navRoot;
    int height;
    struct tripNode* left;
    struct tripNode* right;
} tripNode;

typedef struct tripName {
    char name[50];
    tripNode* root;
} tripName;

tripName list[MAX_TRIPS];
int tripCount = 0;

/* ---------------------------------------------------------------------
   Safe input helpers
   These prevent the classic "scanf fails -> infinite loop" bug that
   happens whenever a user types a letter where a number was expected.
   --------------------------------------------------------------------- */
int readInt() {
    int val;
    while (scanf("%d", &val) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        printf("Invalid input. Please enter a whole number: ");
    }
    return val;
}

float readFloat() {
    float val;
    while (scanf("%f", &val) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        printf("Invalid input. Please enter a number: ");
    }
    return val;
}

/* Reads a line of text into buf (size cap), stripping the trailing
   newline. Also protects against buffer overflow by using a bounded
   format string built from the destination size. */
void readString(char* buf, int size) {
    char fmt[16];
    snprintf(fmt, sizeof(fmt), " %%%d[^\n]", size - 1);
    if (scanf(fmt, buf) != 1) {
        buf[0] = '\0';
    }
}

int max(int a, int b) { return a > b ? a : b; }

/* ======================================================================
   NAVIGATION (per-activity) AVL TREE
   ====================================================================== */

int getNavHeight(navNode* node) {
    return node ? node->height : 0;
}

int getNavBalance(navNode* node) {
    return node ? getNavHeight(node->left) - getNavHeight(node->right) : 0;
}

navNode* createNavNode(int stepId, char dir[], float dist) {
    navNode* n = (navNode*)malloc(sizeof(navNode));
    n->stepId = stepId;
    strncpy(n->Direction, dir, sizeof(n->Direction) - 1);
    n->Direction[sizeof(n->Direction) - 1] = '\0';
    n->Distance = dist;
    n->height = 1;
    n->left = n->right = NULL;
    return n;
}

navNode* rightRotateNav(navNode* y) {
    navNode* x = y->left;
    navNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(getNavHeight(y->left), getNavHeight(y->right)) + 1;
    x->height = max(getNavHeight(x->left), getNavHeight(x->right)) + 1;
    return x;
}

navNode* leftRotateNav(navNode* x) {
    navNode* y = x->right;
    navNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(getNavHeight(x->left), getNavHeight(x->right)) + 1;
    y->height = max(getNavHeight(y->left), getNavHeight(y->right)) + 1;
    return y;
}

navNode* insertNavAVL(navNode* node, navNode* newNav) {
    if (!node) return newNav;

    if (newNav->stepId < node->stepId)
        node->left = insertNavAVL(node->left, newNav);
    else if (newNav->stepId > node->stepId)
        node->right = insertNavAVL(node->right, newNav);
    else
        return node;

    node->height = 1 + max(getNavHeight(node->left), getNavHeight(node->right));
    int balance = getNavBalance(node);

    if (balance > 1 && newNav->stepId < node->left->stepId)
        return rightRotateNav(node);
    if (balance < -1 && newNav->stepId > node->right->stepId)
        return leftRotateNav(node);
    if (balance > 1 && newNav->stepId > node->left->stepId) {
        node->left = leftRotateNav(node->left);
        return rightRotateNav(node);
    }
    if (balance < -1 && newNav->stepId < node->right->stepId) {
        node->right = rightRotateNav(node->right);
        return leftRotateNav(node);
    }
    return node;
}

navNode* minValueNavNode(navNode* node) {
    navNode* current = node;
    while (current->left) current = current->left;
    return current;
}

/* Standard, correct recursive AVL delete. The previous version spliced
   the successor in manually and never rebalanced the nodes on the path
   down to where the successor was removed from, which could silently
   corrupt the AVL balance invariant. This version deletes recursively so
   every ancestor on the path gets its height/balance fixed. */
navNode* deleteNavAVL(navNode* root, int stepId) {
    if (!root) return NULL;

    if (stepId < root->stepId) {
        root->left = deleteNavAVL(root->left, stepId);
    } else if (stepId > root->stepId) {
        root->right = deleteNavAVL(root->right, stepId);
    } else {
        if (!root->left || !root->right) {
            navNode* temp = root->left ? root->left : root->right;
            free(root);
            return temp;
        } else {
            navNode* temp = minValueNavNode(root->right);
            root->stepId = temp->stepId;
            strcpy(root->Direction, temp->Direction);
            root->Distance = temp->Distance;
            root->right = deleteNavAVL(root->right, temp->stepId);
        }
    }

    root->height = 1 + max(getNavHeight(root->left), getNavHeight(root->right));
    int balance = getNavBalance(root);

    if (balance > 1 && getNavBalance(root->left) >= 0)
        return rightRotateNav(root);
    if (balance > 1 && getNavBalance(root->left) < 0) {
        root->left = leftRotateNav(root->left);
        return rightRotateNav(root);
    }
    if (balance < -1 && getNavBalance(root->right) <= 0)
        return leftRotateNav(root);
    if (balance < -1 && getNavBalance(root->right) > 0) {
        root->right = rightRotateNav(root->right);
        return leftRotateNav(root);
    }

    return root;
}

void displayNavInorder(navNode* root) {
    if (root) {
        displayNavInorder(root->left);
        printf("  Step %d: %s", root->stepId, root->Direction);
        if (root->Distance > 0) {
            printf(", Distance: %.1f\n", root->Distance);
        } else {
            printf("\n");
        }
        displayNavInorder(root->right);
    }
}

int getNavCount(navNode* root) {
    if (!root) return 0;
    return 1 + getNavCount(root->left) + getNavCount(root->right);
}

void collectNavNodes(navNode* root, navNode** arr, int* index) {
    if (root) {
        collectNavNodes(root->left, arr, index);
        arr[(*index)++] = root;
        collectNavNodes(root->right, arr, index);
    }
}

void renumberNav(navNode* root, int* counter) {
    if (root) {
        renumberNav(root->left, counter);
        root->stepId = (*counter)++;
        renumberNav(root->right, counter);
    }
}

void freeNavTree(navNode* root) {
    if (root) {
        freeNavTree(root->left);
        freeNavTree(root->right);
        free(root);
    }
}

navNode* rebuildNavTree(navNode** allNodes, int nodeCount) {
    if (nodeCount == 0) return NULL;

    int mid = nodeCount / 2;
    navNode* root = allNodes[mid];

    root->left = rebuildNavTree(allNodes, mid);
    root->right = rebuildNavTree(allNodes + mid + 1, nodeCount - mid - 1);

    root->height = 1 + max(getNavHeight(root->left), getNavHeight(root->right));

    return root;
}

/* ======================================================================
   ACTIVITY AVL TREE
   ====================================================================== */

int getHeight(tripNode* node) {
    return node ? node->height : 0;
}

int getBalance(tripNode* node) {
    return node ? getHeight(node->left) - getHeight(node->right) : 0;
}

int compareDates(char date1[], char date2[]) {
    int d1, m1, y1, d2, m2, y2;
    sscanf(date1, "%d-%d-%d", &d1, &m1, &y1);
    sscanf(date2, "%d-%d-%d", &d2, &m2, &y2);
    if (y1 != y2) return y1 - y2;
    if (m1 != m2) return m1 - m2;
    return d1 - d2;
}

int compareActivities(tripNode* a, tripNode* b) {
    int dateCompare = compareDates(a->date, b->date);
    if (dateCompare != 0) return dateCompare;
    return (a->time < b->time) ? -1 : (a->time > b->time) ? 1 : 0;
}

tripNode* createActivityNode() {
    tripNode* t = (tripNode*)calloc(1, sizeof(tripNode));
    t->height = 1;
    return t;
}

int getActivityCount(tripNode* root) {
    if (!root) return 0;
    return 1 + getActivityCount(root->left) + getActivityCount(root->right);
}

tripNode* rightRotate(tripNode* y) {
    tripNode* x = y->left;
    tripNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    return x;
}

tripNode* leftRotate(tripNode* x) {
    tripNode* y = x->right;
    tripNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    return y;
}

tripNode* insertActivityAVL(tripNode* node, tripNode* newActivity) {
    if (!node) return newActivity;

    int cmp = compareActivities(newActivity, node);
    if (cmp < 0)
        node->left = insertActivityAVL(node->left, newActivity);
    else if (cmp > 0)
        node->right = insertActivityAVL(node->right, newActivity);
    else
        return node;

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));
    int balance = getBalance(node);

    if (balance > 1 && compareActivities(newActivity, node->left) < 0)
        return rightRotate(node);
    if (balance < -1 && compareActivities(newActivity, node->right) > 0)
        return leftRotate(node);
    if (balance > 1 && compareActivities(newActivity, node->left) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && compareActivities(newActivity, node->right) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    return node;
}

tripNode* minValueNode(tripNode* node) {
    tripNode* current = node;
    while (current->left) current = current->left;
    return current;
}

/* Correct recursive AVL delete for activities. Also fixes a real bug in
   the original: when an activity with two children was deleted, the
   in-order successor's navRoot pointer was never transferred, so the
   surviving node kept its OLD (possibly empty) navigation tree while the
   successor's navigation tree either leaked or was freed while still
   referenced. Here ownership of navRoot is explicitly transferred and the
   source pointer is cleared before the successor is recursively removed,
   so there is no leak and no double free. */
tripNode* deleteActivityAVL(tripNode* root, int activityId, int* deleted) {
    if (!root) {
        *deleted = 0;
        return NULL;
    }

    if (activityId < root->activityId) {
        root->left = deleteActivityAVL(root->left, activityId, deleted);
    } else if (activityId > root->activityId) {
        root->right = deleteActivityAVL(root->right, activityId, deleted);
    } else {
        *deleted = 1;
        if (!root->left || !root->right) {
            tripNode* temp = root->left ? root->left : root->right;
            freeNavTree(root->navRoot);
            free(root);
            return temp;
        } else {
            tripNode* temp = minValueNode(root->right);
            root->activityId = temp->activityId;
            root->type = temp->type;
            strcpy(root->date, temp->date);
            root->time = temp->time;
            root->data = temp->data;

            freeNavTree(root->navRoot);
            root->navRoot = temp->navRoot;
            temp->navRoot = NULL; /* ownership transferred, avoid double free */

            root->right = deleteActivityAVL(root->right, temp->activityId, deleted);
        }
    }

    if (!root) return root;

    root->height = 1 + max(getHeight(root->left), getHeight(root->right));
    int balance = getBalance(root);

    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

tripNode* searchActivity(tripNode* root, int activityId) {
    if (!root || root->activityId == activityId) return root;
    return activityId < root->activityId ? searchActivity(root->left, activityId) : searchActivity(root->right, activityId);
}

void updateActivityIds(tripNode* root, int* idCounter) {
    if (root) {
        updateActivityIds(root->left, idCounter);
        root->activityId = (*idCounter)++;
        updateActivityIds(root->right, idCounter);
    }
}

void displayActivitiesInorder(tripNode* root) {
    if (root) {
        displayActivitiesInorder(root->left);

        int hours = (int)root->time;
        int minutes = (int)((root->time - hours) * 100 + 0.5f);
        printf("\nActivity %d: Date: %s Time: %02d:%02d\n",
               root->activityId, root->date, hours, minutes);
        switch (root->type) {
            case 1:
                printf("Flight: %s -> %s\n", root->data.Flight.From, root->data.Flight.To);
                break;
            case 2:
                printf("Hotel: %s, Cost: %d\n", root->data.Hotel.hotelName, root->data.Hotel.cost);
                break;
            case 3:
                printf("Tourist Place: %s\n", root->data.Tourist.place);
                break;
            case 4:
                printf("Transport: %s from %s to %s\n", root->data.Transport.mode,
                       root->data.Transport.from, root->data.Transport.to);
                break;
        }
        if (root->navRoot) {
            printf("Navigation Instructions:\n");
            displayNavInorder(root->navRoot);
        }

        displayActivitiesInorder(root->right);
    }
}

void freeActivityTree(tripNode* root) {
    if (root) {
        freeActivityTree(root->left);
        freeActivityTree(root->right);
        freeNavTree(root->navRoot);
        free(root);
    }
}

/* ======================================================================
   NAVIGATION MENU OPERATIONS
   ====================================================================== */

void addNav(navNode** root) {
    if (getNavCount(*root) >= MAX_NAV_STEPS) {
        printf("Maximum navigation steps (%d) reached for this activity!\n", MAX_NAV_STEPS);
        return;
    }

    char dir[20];
    printf("Direction: ");
    readString(dir, sizeof(dir));
    printf("Distance: ");
    float dist = readFloat();
    if (dist < 0) {
        printf("Distance cannot be negative\n");
        return;
    }

    int count = getNavCount(*root);
    navNode* n = createNavNode(count + 1, dir, dist);
    *root = insertNavAVL(*root, n);
    printf("Navigation instruction added! Step ID: %d\n", count + 1);
}

void insertNavAtPosition(navNode** root, int pos) {
    if (pos < 1) {
        printf("Position must be greater than 0\n");
        return;
    }

    int count = getNavCount(*root);

    if (count >= MAX_NAV_STEPS) {
        printf("Maximum navigation steps (%d) reached for this activity!\n", MAX_NAV_STEPS);
        return;
    }

    if (pos > count + 1) {
        printf("Position %d is invalid. Maximum position is %d\n", pos, count + 1);
        return;
    }

    char dir[20];
    printf("Direction: ");
    readString(dir, sizeof(dir));
    printf("Distance: ");
    float dist = readFloat();
    if (dist < 0) {
        printf("Distance cannot be negative\n");
        return;
    }

    navNode* nodes[MAX_NAV_STEPS];
    int index = 0;
    collectNavNodes(*root, nodes, &index);

    navNode* allNodes[MAX_NAV_STEPS + 1];
    int nodeCount = 0;

    for (int i = 0; i < pos - 1; i++) {
        allNodes[nodeCount++] = nodes[i];
    }

    navNode* newNode = createNavNode(0, dir, dist);
    allNodes[nodeCount++] = newNode;

    for (int i = pos - 1; i < count; i++) {
        allNodes[nodeCount++] = nodes[i];
    }

    for (int i = 0; i < nodeCount; i++) {
        allNodes[i]->stepId = i + 1;
        allNodes[i]->left = NULL;
        allNodes[i]->right = NULL;
        allNodes[i]->height = 1;
    }

    *root = rebuildNavTree(allNodes, nodeCount);

    printf("Navigation instruction added at position %d!\n", pos);
}

void deleteNavByStepId(navNode** root, int stepId) {
    if (!*root) {
        printf("No navigation instructions\n");
        return;
    }

    int before = getNavCount(*root);
    *root = deleteNavAVL(*root, stepId);
    int after = getNavCount(*root);

    if (before == after) {
        printf("Step ID %d not found\n", stepId);
        return;
    }

    int counter = 1;
    renumberNav(*root, &counter);
    printf("Navigation instruction deleted!\n");
}

void searchNavByStepIdHelper(navNode* root, int stepId, navNode** result) {
    if (root && !*result) {
        searchNavByStepIdHelper(root->left, stepId, result);
        if (root->stepId == stepId) *result = root;
        searchNavByStepIdHelper(root->right, stepId, result);
    }
}

void updateNav(navNode** root, int stepId) {
    if (!*root) {
        printf("No navigation instructions\n");
        return;
    }

    navNode* found = NULL;
    searchNavByStepIdHelper(*root, stepId, &found);

    if (!found) {
        printf("Step ID %d not found\n", stepId);
        return;
    }

    char dir[20];
    printf("Enter Direction (current: %s): ", found->Direction);
    readString(dir, sizeof(dir));
    printf("Enter Distance (current: %.1f): ", found->Distance);
    float dist = readFloat();
    if (dist < 0) {
        printf("Distance cannot be negative\n");
        return;
    }

    if (strlen(dir) > 0) strcpy(found->Direction, dir);
    found->Distance = dist;
    printf("Navigation instruction updated!\n");
}

void displayNavMenu(navNode** navRoot) {
    while (1) {
        printf("\n--- Navigation Menu ---\n");
        printf("1. Add Nav (at end)\n");
        printf("2. Insert Nav (at position)\n");
        printf("3. Delete Nav\n");
        printf("4. Update Nav\n");
        printf("5. Display Route\n");
        printf("6. Back\n");
        printf("Choice: ");
        int choice = readInt();

        switch (choice) {
            case 1: addNav(navRoot); break;
            case 2: {
                printf("Position: ");
                int pos = readInt();
                insertNavAtPosition(navRoot, pos);
                break;
            }
            case 3: {
                printf("Step ID to delete: ");
                int delId = readInt();
                deleteNavByStepId(navRoot, delId);
                break;
            }
            case 4: {
                printf("Step ID to update: ");
                int updId = readInt();
                updateNav(navRoot, updId);
                break;
            }
            case 5:
                if (!*navRoot) printf("No navigation instructions\n");
                else displayNavInorder(*navRoot);
                break;
            case 6: return;
            default: printf("Invalid choice\n");
        }
    }
}

/* ======================================================================
   TRIP / ACTIVITY MENU OPERATIONS
   ====================================================================== */

int checkDate(char date[]) {
    int d, m, y;
    if (sscanf(date, "%d-%d-%d", &d, &m, &y) != 3) {
        printf("Invalid date format! Please use dd-mm-yyyy.\n");
        return 0;
    }
    if (y < 2000 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31) {
        printf("Invalid date! Please enter a date between 2000 and 2100 in dd-mm-yyyy format.\n");
        return 0;
    }
    return 1;
}

tripNode* createTripNode() {
    tripNode* t = createActivityNode();
    printf("Date (dd-mm-yyyy): ");
    readString(t->date, sizeof(t->date));
    if (!checkDate(t->date)) {
        free(t);
        return NULL;
    }
    printf("Time (24hr, e.g., 14.30): ");
    t->time = readFloat();

    int hours = (int)t->time;
    int minutes = (int)((t->time - hours) * 100 + 0.5f);
    if (t->time < 0 || t->time >= 24 || minutes >= 60) {
        printf("Invalid time!\n");
        free(t);
        return NULL;
    }

    printf("Type (1-Flight,2-Hotel,3-Tourist,4-Transport): ");
    t->type = readInt();

    switch (t->type) {
        case 1:
            printf("From: ");
            readString(t->data.Flight.From, sizeof(t->data.Flight.From));
            printf("To: ");
            readString(t->data.Flight.To, sizeof(t->data.Flight.To));
            break;
        case 2:
            printf("Hotel: ");
            readString(t->data.Hotel.hotelName, sizeof(t->data.Hotel.hotelName));
            printf("Cost: ");
            t->data.Hotel.cost = readInt();
            if (t->data.Hotel.cost < 0) {
                printf("Cost cannot be negative!\n");
                free(t);
                return NULL;
            }
            break;
        case 3:
            printf("Place: ");
            readString(t->data.Tourist.place, sizeof(t->data.Tourist.place));
            break;
        case 4:
            printf("Mode: ");
            readString(t->data.Transport.mode, sizeof(t->data.Transport.mode));
            printf("From: ");
            readString(t->data.Transport.from, sizeof(t->data.Transport.from));
            printf("To: ");
            readString(t->data.Transport.to, sizeof(t->data.Transport.to));
            break;
        default:
            printf("Invalid type\n");
            free(t);
            return NULL;
    }
    return t;
}

void insertActivity(tripNode** root) {
    if (getActivityCount(*root) >= MAX_ACTIVITIES) {
        printf("Maximum activities (%d) reached for this trip!\n", MAX_ACTIVITIES);
        return;
    }
    tripNode* newNode = createTripNode();
    if (!newNode) return;
    *root = insertActivityAVL(*root, newNode);
    int counter = 1;
    updateActivityIds(*root, &counter);
    printf("Activity added!\n");
}

void deleteActivity(tripNode** root, int activityId) {
    int deleted = 0;
    *root = deleteActivityAVL(*root, activityId, &deleted);
    if (!deleted) {
        printf("Activity %d not found\n", activityId);
        return;
    }
    int counter = 1;
    updateActivityIds(*root, &counter);
    printf("Activity deleted!\n");
}

void displayTrip(tripNode* root) {
    if (!root) printf("No activities\n");
    else displayActivitiesInorder(root);
}

void createTrip() {
    if (tripCount >= MAX_TRIPS) {
        printf("Max trips reached!\n");
        return;
    }
    printf("Enter trip name: ");
    readString(list[tripCount].name, sizeof(list[tripCount].name));
    list[tripCount].root = NULL;
    tripCount++;
    printf("Trip created successfully!\n");
}

void displayTripList() {
    if (tripCount == 0) {
        printf("No trips available\n");
        return;
    }
    printf("\nSr.No\tTrip Name\n");
    printf("----------------\n");
    for (int i = 0; i < tripCount; i++) {
        printf("%d\t%s\n", i + 1, list[i].name);
    }
    printf("----------------\n");
}

void collectActivities(tripNode* root, tripNode** arr, int* count) {
    if (root) {
        collectActivities(root->left, arr, count);
        arr[(*count)++] = root;
        collectActivities(root->right, arr, count);
    }
}

char* getLocation(tripNode* t, int d) {
    switch (t->type) {
        case 1: return d == 1 ? t->data.Flight.From : t->data.Flight.To;
        case 2: return t->data.Hotel.hotelName;
        case 3: return t->data.Tourist.place;
        case 4: return d == 1 ? t->data.Transport.from : t->data.Transport.to;
        default: return "";
    }
}

void collectHotelsHelper(tripNode* root, tripNode** hotels, int* count) {
    if (root) {
        collectHotelsHelper(root->left, hotels, count);
        if (root->type == 2) {
            hotels[(*count)++] = root;
        }
        collectHotelsHelper(root->right, hotels, count);
    }
}

int promptForValidTripNumber() {
    displayTripList();
    if (tripCount == 0) return -1;
    printf("Enter trip number: ");
    int num = readInt();
    if (num < 1 || num > tripCount) {
        printf("Invalid trip number\n");
        return -1;
    }
    return num - 1;
}

void detectDuplicates() {
    int idx = promptForValidTripNumber();
    if (idx < 0) return;

    tripNode* nodes[MAX_ACTIVITIES];
    int nodeCount = 0;
    collectActivities(list[idx].root, nodes, &nodeCount);

    printf("\n--- Checking for Duplicates ---\n");
    int found = 0;

    for (int i = 0; i < nodeCount; i++) {
        if (nodes[i]->type == 3) {
            int count = 1;
            int alreadyPrinted = 0;
            for (int j = 0; j < i; j++) {
                if (nodes[j]->type == 3 && strcmp(nodes[i]->data.Tourist.place, nodes[j]->data.Tourist.place) == 0) {
                    alreadyPrinted = 1;
                    break;
                }
            }
            if (!alreadyPrinted) {
                for (int j = i + 1; j < nodeCount; j++) {
                    if (nodes[j]->type == 3 && strcmp(nodes[i]->data.Tourist.place, nodes[j]->data.Tourist.place) == 0) {
                        count++;
                    }
                }
                if (count > 1) {
                    printf("Tourist place '%s' visited %d times\n", nodes[i]->data.Tourist.place, count);
                    found = 1;
                }
            }
        }
    }

    for (int i = 0; i < nodeCount; i++) {
        if (nodes[i]->type == 2) {
            int count = 1;
            int alreadyPrinted = 0;
            for (int j = 0; j < i; j++) {
                if (nodes[j]->type == 2 && strcmp(nodes[i]->data.Hotel.hotelName, nodes[j]->data.Hotel.hotelName) == 0) {
                    alreadyPrinted = 1;
                    break;
                }
            }
            if (!alreadyPrinted) {
                for (int j = i + 1; j < nodeCount; j++) {
                    if (nodes[j]->type == 2 && strcmp(nodes[i]->data.Hotel.hotelName, nodes[j]->data.Hotel.hotelName) == 0) {
                        count++;
                    }
                }
                if (count > 1) {
                    printf("Hotel '%s' stayed at %d times\n", nodes[i]->data.Hotel.hotelName, count);
                    found = 1;
                }
            }
        }
    }

    if (!found) printf("No duplicates found\n");
}

void sortHotels() {
    int idx = promptForValidTripNumber();
    if (idx < 0) return;

    tripNode* hotels[MAX_ACTIVITIES];
    int hotelCount = 0;
    collectHotelsHelper(list[idx].root, hotels, &hotelCount);

    if (hotelCount == 0) {
        printf("No hotels found\n");
        return;
    }

    for (int i = 1; i < hotelCount; i++) {
        tripNode* key = hotels[i];
        int j = i - 1;

        while (j >= 0 && hotels[j]->data.Hotel.cost < key->data.Hotel.cost) {
            hotels[j + 1] = hotels[j];
            j = j - 1;
        }
        hotels[j + 1] = key;
    }

    printf("\nHotels by cost (highest to lowest):\n");
    printf("----------------------------------------\n");
    for (int i = 0; i < hotelCount; i++)
        printf("%d. %s - Cost: %d\n", i + 1, hotels[i]->data.Hotel.hotelName, hotels[i]->data.Hotel.cost);
    printf("----------------------------------------\n");
}

void searchNavigation() {
    int idx = promptForValidTripNumber();
    if (idx < 0) return;

    tripNode* root = list[idx].root;
    if (!root) {
        printf("No activities in this trip\n");
        return;
    }

    char source[50], dest[50];
    printf("Enter Source: ");
    readString(source, sizeof(source));
    printf("Enter Destination: ");
    readString(dest, sizeof(dest));

    tripNode* inorderList[MAX_ACTIVITIES];
    int count = 0;
    collectActivities(root, inorderList, &count);

    int start = -1, end = -1;

    for (int i = 0; i < count; i++) {
        if (strcmp(getLocation(inorderList[i], 1), source) == 0) {
            start = i;
            break;
        }
    }

    if (start == -1) {
        printf("Source '%s' not found\n", source);
        return;
    }

    for (int i = start; i < count; i++) {
        if (strcmp(getLocation(inorderList[i], 2), dest) == 0) {
            end = i;
            break;
        }
    }

    if (end == -1) {
        printf("No route found from %s to %s\n", source, dest);
        return;
    }

    printf("\nRoute from %s to %s\n", source, dest);
    printf("\nPath:\n");
    int step = 1;
    printf("\nStep %d: Start at %s\n", step++, getLocation(inorderList[start], 1));

    for (int i = start; i <= end; i++) {
        tripNode* curr = inorderList[i];
        switch (curr->type) {
            case 1:
                printf("Take flight from %s to %s\n", curr->data.Flight.From, curr->data.Flight.To);
                break;
            case 2:
                printf("Stay at %s hotel\n", curr->data.Hotel.hotelName);
                break;
            case 3:
                printf("Visit tourist place: %s\n", curr->data.Tourist.place);
                break;
            case 4:
                printf("Take %s from %s to %s\n", curr->data.Transport.mode,
                       curr->data.Transport.from, curr->data.Transport.to);
                break;
        }
        if (curr->navRoot) {
            printf("Navigation directions:\n");
            displayNavInorder(curr->navRoot);
        }
        printf("Arrive at: %s\n", getLocation(curr, 2));

        if (i < end) {
            printf("\nStep %d: Continue from %s\n", step++, getLocation(inorderList[i + 1], 1));
        }
    }
    printf("\n=== Destination Reached! ===\n");
}

void searchDirectionInNav(navNode* root, char direction[], navNode** result) {
    if (root && !*result) {
        searchDirectionInNav(root->left, direction, result);
        if (strcmp(root->Direction, direction) == 0) *result = root;
        searchDirectionInNav(root->right, direction, result);
    }
}

/* Fixed range-search logic: previously, if every remaining activity fell
   within [date1, date2] (i.e. none exceeded date2), `en` was left at -1
   and the function incorrectly reported "End date Not found!" instead of
   including all matching activities through the end of the list. */
void rangeSearch(tripNode* root, char date1[], char date2[]) {
    if (!root) {
        printf("No activities in this trip\n");
        return;
    }

    tripNode* inorderList[MAX_ACTIVITIES];
    int count = 0;
    collectActivities(root, inorderList, &count);

    int st = -1, en = -1;

    for (int i = 0; i < count; i++) {
        if (compareDates(inorderList[i]->date, date1) >= 0) {
            st = i;
            break;
        }
    }

    if (st == -1) {
        printf("No activities found in the given date range\n");
        return;
    }

    en = count - 1;
    for (int i = st; i < count; i++) {
        if (compareDates(inorderList[i]->date, date2) > 0) {
            en = i - 1;
            break;
        }
    }

    if (en < st) {
        printf("No activities found in the given date range\n");
        return;
    }

    printf("\n--- Activities from %s to %s ---\n", date1, date2);
    for (int i = st; i <= en; i++) {
        printf("\nActivity %d: Date: %s Time: %.2f\n",
               inorderList[i]->activityId, inorderList[i]->date, inorderList[i]->time);
        switch (inorderList[i]->type) {
            case 1:
                printf("Flight: %s -> %s\n", inorderList[i]->data.Flight.From, inorderList[i]->data.Flight.To);
                break;
            case 2:
                printf("Hotel: %s, Cost: %d\n", inorderList[i]->data.Hotel.hotelName, inorderList[i]->data.Hotel.cost);
                break;
            case 3:
                printf("Tourist Place: %s\n", inorderList[i]->data.Tourist.place);
                break;
            case 4:
                printf("Transport: %s from %s to %s\n", inorderList[i]->data.Transport.mode,
                       inorderList[i]->data.Transport.from, inorderList[i]->data.Transport.to);
                break;
        }
    }
}

void searchNavigationInstruction() {
    int idx = promptForValidTripNumber();
    if (idx < 0) return;

    tripNode* root = list[idx].root;
    if (!root) {
        printf("No activities in this trip\n");
        return;
    }

    displayTrip(root);
    printf("Enter activity number: ");
    int actId = readInt();

    tripNode* act = searchActivity(root, actId);
    if (!act) {
        printf("Activity %d not found\n", actId);
        return;
    }

    if (!act->navRoot) {
        printf("No navigation instructions for this activity\n");
        return;
    }

    printf("\nNavigation Instructions for Activity %d:\n", actId);
    displayNavInorder(act->navRoot);

    char dir[20];
    printf("\nEnter navigation direction to search (eg: Straight, Right, Left): ");
    readString(dir, sizeof(dir));

    navNode* found = NULL;
    searchDirectionInNav(act->navRoot, dir, &found);

    if (found) {
        printf("\nFound - Step %d: %s -> %.1f km\n", found->stepId, found->Direction, found->Distance);
    } else {
        printf("Direction '%s' not found\n", dir);
    }
}

void freeAll() {
    for (int i = 0; i < tripCount; i++) {
        freeActivityTree(list[i].root);
    }
}

/* ======================================================================
   FILE HANDLING (new)
   Trips are stored in a plain-text file so they persist between runs.
   Both trees are serialized via their in-order traversal (which is
   already sorted), then rebuilt on load with the normal AVL insert
   functions, so the file format doesn't need to know anything about
   tree shape.
   ====================================================================== */

void writeStringLine(FILE* fp, const char* str) {
    fprintf(fp, "%s\n", str);
}

int readFileLine(FILE* fp, char* buf, int size) {
    if (!fgets(buf, size, fp)) {
        buf[0] = '\0';
        return 0;
    }
    int len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return 1;
}

void saveNavTreeToFile(FILE* fp, navNode* navRoot) {
    navNode* navs[MAX_NAV_STEPS];
    int navCount = 0;
    collectNavNodes(navRoot, navs, &navCount);
    fprintf(fp, "%d\n", navCount);
    for (int j = 0; j < navCount; j++) {
        fprintf(fp, "%d\n", navs[j]->stepId);
        writeStringLine(fp, navs[j]->Direction);
        fprintf(fp, "%.2f\n", navs[j]->Distance);
    }
}

void saveTripToFile(FILE* fp, tripName* trip) {
    writeStringLine(fp, trip->name);

    tripNode* nodes[MAX_ACTIVITIES];
    int count = 0;
    collectActivities(trip->root, nodes, &count);
    fprintf(fp, "%d\n", count);

    for (int i = 0; i < count; i++) {
        tripNode* t = nodes[i];
        fprintf(fp, "%d\n", t->type);
        writeStringLine(fp, t->date);
        fprintf(fp, "%.2f\n", t->time);

        switch (t->type) {
            case 1:
                writeStringLine(fp, t->data.Flight.From);
                writeStringLine(fp, t->data.Flight.To);
                break;
            case 2:
                writeStringLine(fp, t->data.Hotel.hotelName);
                fprintf(fp, "%d\n", t->data.Hotel.cost);
                break;
            case 3:
                writeStringLine(fp, t->data.Tourist.place);
                break;
            case 4:
                writeStringLine(fp, t->data.Transport.mode);
                writeStringLine(fp, t->data.Transport.from);
                writeStringLine(fp, t->data.Transport.to);
                break;
        }

        saveNavTreeToFile(fp, t->navRoot);
    }
}

int saveAllTrips() {
    FILE* fp = fopen(DATA_FILE, "w");
    if (!fp) {
        printf("Error: could not open '%s' for saving!\n", DATA_FILE);
        return 0;
    }

    fprintf(fp, "%d\n", tripCount);
    for (int i = 0; i < tripCount; i++) {
        saveTripToFile(fp, &list[i]);
    }

    fclose(fp);
    printf("Trips saved to '%s' successfully!\n", DATA_FILE);
    return 1;
}

navNode* loadNavTreeFromFile(FILE* fp) {
    int navCount = 0;
    char buf[32];
    if (!readFileLine(fp, buf, sizeof(buf))) return NULL;
    navCount = atoi(buf);

    navNode* navRoot = NULL;
    for (int j = 0; j < navCount; j++) {
        char stepBuf[32], dir[20], distBuf[32];
        readFileLine(fp, stepBuf, sizeof(stepBuf));
        readFileLine(fp, dir, sizeof(dir));
        readFileLine(fp, distBuf, sizeof(distBuf));

        int stepId = atoi(stepBuf);
        float dist = (float)atof(distBuf);

        navNode* n = createNavNode(stepId, dir, dist);
        navRoot = insertNavAVL(navRoot, n);
    }
    return navRoot;
}

int loadTripFromFile(FILE* fp, tripName* trip) {
    char nameBuf[50];
    if (!readFileLine(fp, nameBuf, sizeof(nameBuf))) return 0;
    strcpy(trip->name, nameBuf);
    trip->root = NULL;

    char countBuf[32];
    if (!readFileLine(fp, countBuf, sizeof(countBuf))) return 0;
    int count = atoi(countBuf);

    for (int i = 0; i < count; i++) {
        tripNode* t = createActivityNode();

        char typeBuf[16];
        readFileLine(fp, typeBuf, sizeof(typeBuf));
        t->type = atoi(typeBuf);

        readFileLine(fp, t->date, sizeof(t->date));

        char timeBuf[32];
        readFileLine(fp, timeBuf, sizeof(timeBuf));
        t->time = (float)atof(timeBuf);

        switch (t->type) {
            case 1:
                readFileLine(fp, t->data.Flight.From, sizeof(t->data.Flight.From));
                readFileLine(fp, t->data.Flight.To, sizeof(t->data.Flight.To));
                break;
            case 2: {
                readFileLine(fp, t->data.Hotel.hotelName, sizeof(t->data.Hotel.hotelName));
                char costBuf[32];
                readFileLine(fp, costBuf, sizeof(costBuf));
                t->data.Hotel.cost = atoi(costBuf);
                break;
            }
            case 3:
                readFileLine(fp, t->data.Tourist.place, sizeof(t->data.Tourist.place));
                break;
            case 4:
                readFileLine(fp, t->data.Transport.mode, sizeof(t->data.Transport.mode));
                readFileLine(fp, t->data.Transport.from, sizeof(t->data.Transport.from));
                readFileLine(fp, t->data.Transport.to, sizeof(t->data.Transport.to));
                break;
        }

        t->navRoot = loadNavTreeFromFile(fp);

        trip->root = insertActivityAVL(trip->root, t);
    }

    int counter = 1;
    updateActivityIds(trip->root, &counter);
    return 1;
}

/* Returns 1 if trips were loaded from an existing file, 0 if there was
   no save file yet (first run). */
int loadAllTrips() {
    FILE* fp = fopen(DATA_FILE, "r");
    if (!fp) return 0;

    char countBuf[32];
    if (!readFileLine(fp, countBuf, sizeof(countBuf))) {
        fclose(fp);
        return 0;
    }
    int count = atoi(countBuf);
    if (count > MAX_TRIPS) count = MAX_TRIPS;

    tripCount = 0;
    for (int i = 0; i < count; i++) {
        if (!loadTripFromFile(fp, &list[tripCount])) break;
        tripCount++;
    }

    fclose(fp);
    printf("Trips loaded from '%s' successfully!\n", DATA_FILE);
    return 1;
}

void exitProgram() {
    printf("Save trips before exiting? (1-Yes, 0-No): ");
    int choice = readInt();
    if (choice == 1) saveAllTrips();
    freeAll();
    exit(0);
}

/* ======================================================================
   DEFAULT SAMPLE DATA (used only when no save file exists yet)
   ====================================================================== */

void initData() {
    strcpy(list[0].name, "Goa Trip");
    list[0].root = NULL;
    tripCount = 1;

    tripNode* t1 = createActivityNode();
    strcpy(t1->date, "01-01-2024");
    t1->type = 4;
    t1->time = 9.00;
    strcpy(t1->data.Transport.mode, "Car");
    strcpy(t1->data.Transport.from, "Home");
    strcpy(t1->data.Transport.to, "Airport");
    t1->navRoot = createNavNode(1, "Straight", 5);
    t1->navRoot = insertNavAVL(t1->navRoot, createNavNode(2, "Left", 2));
    t1->navRoot = insertNavAVL(t1->navRoot, createNavNode(3, "Right", 3));
    list[0].root = insertActivityAVL(list[0].root, t1);

    tripNode* t2 = createActivityNode();
    strcpy(t2->date, "02-01-2024");
    t2->type = 1;
    t2->time = 14.30;
    strcpy(t2->data.Flight.From, "Hyderabad");
    strcpy(t2->data.Flight.To, "Goa");
    list[0].root = insertActivityAVL(list[0].root, t2);

    tripNode* t3 = createActivityNode();
    strcpy(t3->date, "03-01-2024");
    t3->type = 4;
    t3->time = 16.00;
    strcpy(t3->data.Transport.mode, "Car");
    strcpy(t3->data.Transport.from, "Airport");
    strcpy(t3->data.Transport.to, "GoaHotel");
    t3->navRoot = createNavNode(1, "Straight", 5);
    t3->navRoot = insertNavAVL(t3->navRoot, createNavNode(2, "Left", 2));
    t3->navRoot = insertNavAVL(t3->navRoot, createNavNode(3, "Right", 3));
    list[0].root = insertActivityAVL(list[0].root, t3);

    tripNode* t4 = createActivityNode();
    strcpy(t4->date, "04-01-2024");
    t4->type = 2;
    t4->time = 18.00;
    strcpy(t4->data.Hotel.hotelName, "BeachResort");
    t4->data.Hotel.cost = 5000;
    t4->navRoot = createNavNode(1, "Straight", 5);
    t4->navRoot = insertNavAVL(t4->navRoot, createNavNode(2, "Left", 2));
    t4->navRoot = insertNavAVL(t4->navRoot, createNavNode(3, "Right", 3));
    list[0].root = insertActivityAVL(list[0].root, t4);

    tripNode* t5 = createActivityNode();
    strcpy(t5->date, "05-01-2024");
    t5->type = 3;
    t5->time = 10.00;
    strcpy(t5->data.Tourist.place, "BagaBeach");
    t5->navRoot = createNavNode(1, "Straight", 5);
    t5->navRoot = insertNavAVL(t5->navRoot, createNavNode(2, "Left", 2));
    t5->navRoot = insertNavAVL(t5->navRoot, createNavNode(3, "Right", 3));
    list[0].root = insertActivityAVL(list[0].root, t5);

    tripNode* t6 = createActivityNode();
    strcpy(t6->date, "06-01-2024");
    t6->type = 2;
    t6->time = 18.00;
    strcpy(t6->data.Hotel.hotelName, "CityHotel");
    t6->data.Hotel.cost = 3000;
    t6->navRoot = createNavNode(1, "Straight", 5);
    t6->navRoot = insertNavAVL(t6->navRoot, createNavNode(2, "Left", 2));
    t6->navRoot = insertNavAVL(t6->navRoot, createNavNode(3, "Right", 3));
    list[0].root = insertActivityAVL(list[0].root, t6);

    tripNode* t7 = createActivityNode();
    strcpy(t7->date, "07-01-2024");
    t7->type = 3;
    t7->time = 10.00;
    strcpy(t7->data.Tourist.place, "BagaBeach");
    t7->navRoot = createNavNode(1, "Straight", 5);
    t7->navRoot = insertNavAVL(t7->navRoot, createNavNode(2, "Left", 2));
    t7->navRoot = insertNavAVL(t7->navRoot, createNavNode(3, "Right", 3));
    list[0].root = insertActivityAVL(list[0].root, t7);

    int counter = 1;
    updateActivityIds(list[0].root, &counter);
}

/* ======================================================================
   EDIT TRIP MENU
   ====================================================================== */

void editTrip() {
    int idx = promptForValidTripNumber();
    if (idx < 0) return;

    printf("\n--- Editing Trip: %s ---\n", list[idx].name);
    printf("1. Add Activity\n2. Delete Activity\n3. Edit Navigation\n4. Delete Trip\n5. Back\n");
    printf("Choice: ");
    int choice = readInt();

    switch (choice) {
        case 1:
            insertActivity(&list[idx].root);
            break;
        case 2:
            if (!list[idx].root) {
                printf("No activities\n");
            } else {
                displayTrip(list[idx].root);
                printf("Activity ID to delete: ");
                int id = readInt();
                deleteActivity(&list[idx].root, id);
            }
            break;
        case 3:
            if (!list[idx].root) {
                printf("No activities\n");
            } else {
                displayTrip(list[idx].root);
                printf("Activity ID for navigation: ");
                int id = readInt();
                tripNode* act = searchActivity(list[idx].root, id);
                if (act) {
                    displayNavMenu(&act->navRoot);
                } else {
                    printf("Activity not found\n");
                }
            }
            break;
        case 4:
            freeActivityTree(list[idx].root);
            for (int i = idx; i < tripCount - 1; i++) {
                list[i] = list[i + 1];
            }
            tripCount--;
            printf("Trip deleted successfully!\n");
            break;
        case 5:
            return;
        default:
            printf("Invalid choice\n");
    }
}

/* ======================================================================
   MAIN
   ====================================================================== */

int main() {
    if (!loadAllTrips()) {
        initData();
    }

    while (1) {
        printf("\n========== MAIN MENU ==========\n");
        printf("1. Create Trip\n");
        printf("2. Display Trip\n");
        printf("3. Display All Trips\n");
        printf("4. Edit Trip\n");
        printf("5. Detect Duplicates\n");
        printf("6. Sort Hotels\n");
        printf("7. Search Navigation\n");
        printf("8. Search Navigation by Direction\n");
        printf("9. Range Search\n");
        printf("10. Save Trips to File\n");
        printf("11. Load Trips from File\n");
        printf("12. Exit\n");
        printf("===============================\n");
        printf("Choice: ");

        int choice = readInt();

        switch (choice) {
            case 1:
                createTrip();
                break;
            case 2: {
                int idx = promptForValidTripNumber();
                if (idx >= 0) {
                    printf("\n========== Trip Name: %s ==========\n", list[idx].name);
                    displayTrip(list[idx].root);
                    printf("===================================\n");
                }
                break;
            }
            case 3: {
                for (int i = 0; i < tripCount; i++) {
                    printf("\n========== Trip Name: %s ==========\n", list[i].name);
                    displayTrip(list[i].root);
                    printf("===================================\n");
                }
                break;
            }
            case 4:
                editTrip();
                break;
            case 5:
                detectDuplicates();
                break;
            case 6:
                sortHotels();
                break;
            case 7:
                searchNavigation();
                break;
            case 8:
                searchNavigationInstruction();
                break;
            case 9: {
                int idx = promptForValidTripNumber();
                if (idx >= 0) {
                    tripNode* root = list[idx].root;
                    char date1[15];
                    char date2[15];
                    printf("Enter Start Date (dd-mm-yyyy): ");
                    readString(date1, sizeof(date1));
                    printf("Enter End Date (dd-mm-yyyy): ");
                    readString(date2, sizeof(date2));
                    rangeSearch(root, date1, date2);
                }
                break; /* BUG FIX: this break was missing, so the program
                          used to fall through into case 10 (Exit) every
                          time Range Search ran. */
            }
            case 10:
                saveAllTrips();
                break;
            case 11:
                if (loadAllTrips()) {
                    /* reloaded successfully */
                } else {
                    printf("No save file found ('%s').\n", DATA_FILE);
                }
                break;
            case 12:
                printf("\nThank you!\n");
                exitProgram();
                break;
            default:
                printf("Invalid choice! Please enter 1-12\n");
        }
    }
    return 0;
}
