#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct navNode {
    char Direction[20];
    float Distance;
    struct navNode* next;
}navNode;

typedef struct fNode {
    char From[50];
    char To[50];
}flight;

typedef struct hNode {
    char hotelName[50];
    int cost;
}hotel;

typedef struct tNode {
    char place[50];
}tourist;

typedef struct trNode {
    char mode[50];
    char from[50];
    char to[50];
}transport;

typedef union {
    flight Flight;
    hotel Hotel;
    tourist Tourist;
    transport Transport;
}ativity;

typedef struct tripNode {
    int activityId;
    int type;
    char date[15];
    float time;
    ativity data;
    navNode* navHead;
    int height;
    struct tripNode* left;
    struct tripNode* right;
}tripNode;

typedef struct tripName {
    char name[50];
    tripNode* root;
    struct tripName* next;
}tripName;

tripName* tripList = NULL;
int tripCount = 0;

// Global arrays for collecting nodes
tripNode* nodes[100];
int nodeCount = 0;

// Function declarations
void collectNodes(tripNode* root);
void collectHotels(tripNode* root, tripNode** hotels, int* hotelCount);
void getInorderList(tripNode* root, tripNode** list, int* index);

// AVL Tree utility functions for activities
int max(int a, int b) {
    return (a > b) ? a : b;
}

int getHeight(tripNode* node) {
    if (node == NULL)
        return 0;
    return node->height;
}

int getBalance(tripNode* node) {
    if (node == NULL)
        return 0;
    return getHeight(node->left) - getHeight(node->right);
}

// Compare two activities by date then time
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
    if (dateCompare != 0)
        return dateCompare;
    if (a->time < b->time) return -1;
    if (a->time > b->time) return 1;
    return 0;
}

tripNode* createActivityNode() {
    tripNode* t = (tripNode*)malloc(sizeof(tripNode));
    t->left = NULL;
    t->right = NULL;
    t->height = 1;
    t->navHead = NULL;
    return t;
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
    if (node == NULL) {
        return newActivity;
    }

    int cmp = compareActivities(newActivity, node);
    
    if (cmp < 0)
        node->left = insertActivityAVL(node->left, newActivity);
    else if (cmp > 0)
        node->right = insertActivityAVL(node->right, newActivity);
    else
        return node;

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getBalance(node);

    // Left Left Case
    if (balance > 1 && compareActivities(newActivity, node->left) < 0)
        return rightRotate(node);

    // Right Right Case
    if (balance < -1 && compareActivities(newActivity, node->right) > 0)
        return leftRotate(node);

    // Left Right Case
    if (balance > 1 && compareActivities(newActivity, node->left) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // Right Left Case
    if (balance < -1 && compareActivities(newActivity, node->right) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

tripNode* minValueNode(tripNode* node) {
    tripNode* current = node;
    while (current && current->left != NULL)
        current = current->left;
    return current;
}

tripNode* deleteActivityAVL(tripNode* root, int activityId, int* deleted) {
    if (root == NULL) {
        *deleted = 0;
        return NULL;
    }

    if (activityId < root->activityId)
        root->left = deleteActivityAVL(root->left, activityId, deleted);
    else if (activityId > root->activityId)
        root->right = deleteActivityAVL(root->right, activityId, deleted);
    else {
        *deleted = 1;
        if ((root->left == NULL) || (root->right == NULL)) {
            tripNode* temp = root->left ? root->left : root->right;
            if (temp == NULL) {
                temp = root;
                root = NULL;
            } else {
                *root = *temp;
            }
            freeNav(temp->navHead);
            free(temp);
        } else {
            tripNode* temp = minValueNode(root->right);
            root->activityId = temp->activityId;
            root->type = temp->type;
            strcpy(root->date, temp->date);
            root->time = temp->time;
            root->data = temp->data;
            root->navHead = temp->navHead;
            temp->navHead = NULL;
            root->right = deleteActivityAVL(root->right, temp->activityId, deleted);
        }
    }

    if (root == NULL)
        return root;

    root->height = 1 + max(getHeight(root->left), getHeight(root->right));

    int balance = getBalance(root);

    // Left Left Case
    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);

    // Left Right Case
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    // Right Right Case
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);

    // Right Left Case
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

tripNode* searchActivity(tripNode* root, int activityId) {
    if (root == NULL || root->activityId == activityId)
        return root;
    
    if (activityId < root->activityId)
        return searchActivity(root->left, activityId);
    
    return searchActivity(root->right, activityId);
}

void inorderTraversal(tripNode* root, int* idCounter) {
    if (root != NULL) {
        inorderTraversal(root->left, idCounter);
        root->activityId = (*idCounter)++;
        inorderTraversal(root->right, idCounter);
    }
}

void updateActivityIds(tripNode* root) {
    int idCounter = 1;
    inorderTraversal(root, &idCounter);
}

void displayActivitiesInorder(tripNode* root) {
    if (root != NULL) {
        displayActivitiesInorder(root->left);
        
        int hours = (int)root->time;
        int minutes = (int)((root->time - hours) * 100);
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
                printf("Transport Mode: %s\n", root->data.Transport.mode);
                printf("From: %s\n", root->data.Transport.from);
                printf("To: %s\n", root->data.Transport.to);
                break;
        }
        if(root->navHead) {
            printf("Route: ");
            displayNav(root->navHead);
        }
        
        displayActivitiesInorder(root->right);
    }
}

void freeActivityTree(tripNode* root) {
    if (root != NULL) {
        freeActivityTree(root->left);
        freeActivityTree(root->right);
        freeNav(root->navHead);
        free(root);
    }
}

navNode* createNav(char dir[], float dist) {
    navNode* n = (navNode*)malloc(sizeof(navNode));
    strcpy(n->Direction, dir);
    n->Distance = dist;
    n->next = NULL;
    return n;
}

void addNav(navNode** head) {
    char dir[50]; float dist;
    printf("Direction: ");
    scanf("%s", dir);
    printf("\n");
    printf("Distance: ");
    scanf("%f", &dist);
    if(dist < 0) {
        printf("Distance cannot be negative\n");
        return;
    }
    printf("\n");
    navNode* n = createNav(dir, dist);
    if (*head == NULL) {
        *head = n;
        return;
    }
    navNode* t = *head;
    while (t->next)
        t = t->next;
    t->next = n;
}

void insertNav(navNode** head, int pos) {
    if(pos < 1) {
        printf("Position must be greater than 0\n");
        return;
    }
    char dir[50]; float dist;
    printf("Direction: ");
    scanf("%s", dir);
    printf("\n");
    printf("Distance: ");
    scanf("%f", &dist);
    if(dist < 0) {
        printf("Distance cannot be negative\n");
        return;
    }
    printf("\n");
    navNode* n = createNav(dir, dist);
    if (pos == 1) {
        n->next = *head;
        *head = n;
        printf("Navigation instruction added successfully at position %d!\n", pos);
        return;
    }
    navNode* t = *head;
    for (int i = 1; t && i < pos - 1; i++)
        t = t->next;
    if (!t) {
        printf("Position out of bounds\n");
        free(n);
        return;
    }
    n->next = t->next;
    t->next = n;
    printf("Navigation instruction added successfully at position %d!\n", pos);
}

void deleteNav(navNode** head, int pos) {
    if (*head == NULL) return;
    navNode* temp = *head;

    if (pos == 1) {
        *head = temp->next;
        free(temp);
        printf("Navigation instruction deleted successfully!\n");
        return;
    }

    for (int i = 1; temp->next && i < pos - 1; i++)
        temp = temp->next;

    if (temp->next == NULL) {
        printf("Position out of bounds\n");
        return;
    }
    navNode* toDelete = temp->next;
    temp->next = temp->next->next;
    printf("Navigation instruction deleted successfully!\n");
    free(toDelete);
}

void updateNav(navNode** head , int pos) {
    if(*head == NULL) return;
    navNode* temp = *head;
    for(int i = 1 ; temp && i < pos ; i++) {
        temp = temp->next;
    }
    if(temp == NULL) {
        printf("Position out of bounds\n");
        return;
    }
    char dir[50]; float dist;
    printf("Enter Direction: ");
    scanf("%s", dir);
    printf("Enter Distance: ");
    scanf("%f", &dist);
    if(dist < 0) {
        printf("Distance cannot be negative\n");
        return;
    }
    if(strlen(dir) > 0) {
        strcpy(temp->Direction, dir);
    }
    temp->Distance = dist;
    printf("Navigation instruction updated successfully!\n");
}

void displayNav(navNode* head) {
    while(head) {
        printf(" %s ", head->Direction);
        if(head->Distance > 0) {
            printf("(%.1f km)", head->Distance);
        }
        if(head->next) {
            printf(" -> ");
        }
        head = head->next;
    }
    printf("\n");
}

void freeNav(navNode* head) {
    while(head) {
        navNode* temp = head;
        head = head->next;
        free(temp);
    }
}

void freeAll() {
    tripName* current = tripList;
    while(current) {
        freeActivityTree(current->root);
        tripName* temp = current;
        current = current->next;
        free(temp);
    }
}

void exitProgram() {
    freeAll();
    exit(0);
}

tripNode* createTripNode() {
    tripNode* t = createActivityNode();
    printf("Enter date (dd-mm-yyyy): ");
    scanf("%s", t->date);
    printf("Enter time (in 24hr format, e.g., 14.30 for 2:30 PM): ");
    scanf("%f", &t->time);
    
    int hours = (int)t->time;
    int minutes = (int)((t->time - hours) * 100);
    
    if(t->time < 0.0 || t->time >= 24.0 || minutes >= 60) {
        printf("Invalid time entered! Hours: 0-23, Minutes: 0-59\n");
        free(t);
        return NULL;
    }
    
    printf("Enter type (1-Flight, 2-Hotel, 3-Tourist, 4-Transport): ");
    scanf("%d", &t->type);

    switch (t->type) {
        case 1:
            printf("From: ");
            scanf("%s", t->data.Flight.From);
            printf("To: ");
            scanf("%s", t->data.Flight.To);
            break;
        case 2:
            printf("Hotel Name: ");
            scanf("%s", t->data.Hotel.hotelName);
            printf("Cost: ");
            scanf("%d", &t->data.Hotel.cost);
            break;
        case 3:
            printf("Place: ");
            scanf("%s", t->data.Tourist.place);
            break;
        case 4:
            printf("Mode of Transport: ");
            scanf("%s", t->data.Transport.mode);
            printf("From: ");
            scanf("%s", t->data.Transport.from);
            printf("To: ");
            scanf("%s", t->data.Transport.to);
            break;
        default:
            printf("Invalid type\n");
            free(t);
            return NULL;
    }
    return t;
}

void insertActivity(tripNode** root) {
    tripNode* newNode = createTripNode();
    if (!newNode) return;

    *root = insertActivityAVL(*root, newNode);
    updateActivityIds(*root);
    printf("Activity added successfully!\n");
}

void deleteActivity(tripNode** root, int activityId) {
    int deleted = 0;
    *root = deleteActivityAVL(*root, activityId, &deleted);
    
    if (!deleted) {
        printf("Activity with ID %d not found\n", activityId);
        return;
    }
    
    updateActivityIds(*root);
    printf("Activity deleted successfully!\n");
}

void displayTrip(tripNode* root) {
    if(!root) {
        printf("No activities in this trip\n");
        return;
    }
    displayActivitiesInorder(root);
}

void createTrip() {
    if(tripCount >= 10) {
        printf("Maximum trips reached!\n");
        return;
    }
    
    tripName* newTrip = (tripName*)malloc(sizeof(tripName));
    printf("Enter trip name: ");
    scanf("%s", newTrip->name);
    newTrip->root = NULL;
    newTrip->next = tripList;
    tripList = newTrip;
    tripCount++;
    printf("Trip created successfully!\n");
}

void displayTripList() {
    if(tripCount == 0) {
        printf("No trips available\n");
        return;
    }
    printf("\nSr.No\tTrip Name\n");
    printf("----------------\n");
    int i = 1;
    tripName* current = tripList;
    while(current) {
        printf("%d\t%s\n", i++, current->name);
        current = current->next;
    }
    printf("----------------\n");
}

tripName* findTrip(int num) {
    if(num < 1) return NULL;
    tripName* current = tripList;
    for(int i = 1; current && i < num; i++) {
        current = current->next;
    }
    return current;
}

// Helper function to collect all nodes in inorder
void collectNodes(tripNode* root) {
    if(root) {
        collectNodes(root->left);
        nodes[nodeCount++] = root;
        collectNodes(root->right);
    }
}

// Helper function to collect hotel nodes
void collectHotels(tripNode* root, tripNode** hotels, int* hotelCount) {
    if(root) {
        collectHotels(root->left, hotels, hotelCount);
        if(root->type == 2) {
            hotels[(*hotelCount)++] = root;
        }
        collectHotels(root->right, hotels, hotelCount);
    }
}

// Helper function to get inorder list
void getInorderList(tripNode* root, tripNode** list, int* index) {
    if(root) {
        getInorderList(root->left, list, index);
        list[(*index)++] = root;
        getInorderList(root->right, list, index);
    }
}

void detectDuplicates() {
    displayTripList();
    printf("Enter trip number to check for duplicates: ");
    int num;
    scanf("%d", &num);
    tripName* trip = findTrip(num);
    
    if(!trip) {
        printf("Invalid trip number\n");
        return;
    }
    
    int found = 0;
    
    // Reset node count
    nodeCount = 0;
    
    // Collect all nodes
    collectNodes(trip->root);
    
    printf("\n--- Checking for Duplicates ---\n");
    
    // Check for duplicate tourist places
    for(int i = 0; i < nodeCount; i++) {
        if(nodes[i]->type == 3) { // Tourist place
            int alreadyChecked = 0;
            for(int k = 0; k < i; k++) {
                if(nodes[k]->type == 3 && strcmp(nodes[k]->data.Tourist.place, nodes[i]->data.Tourist.place) == 0) {
                    alreadyChecked = 1;
                    break;
                }
            }
            if(alreadyChecked) continue;
            
            int count = 1;
            for(int j = i+1; j < nodeCount; j++) {
                if(nodes[j]->type == 3 && strcmp(nodes[i]->data.Tourist.place, nodes[j]->data.Tourist.place) == 0) {
                    count++;
                }
            }
            if(count > 1) {
                printf("Tourist place '%s' appears %d times\n", nodes[i]->data.Tourist.place, count);
                found = 1;
            }
        }
    }
    
    // Check for duplicate hotels
    for(int i = 0; i < nodeCount; i++) {
        if(nodes[i]->type == 2) { // Hotel
            int alreadyChecked = 0;
            for(int k = 0; k < i; k++) {
                if(nodes[k]->type == 2 && strcmp(nodes[k]->data.Hotel.hotelName, nodes[i]->data.Hotel.hotelName) == 0) {
                    alreadyChecked = 1;
                    break;
                }
            }
            if(alreadyChecked) continue;
            
            int count = 1;
            for(int j = i+1; j < nodeCount; j++) {
                if(nodes[j]->type == 2 && strcmp(nodes[i]->data.Hotel.hotelName, nodes[j]->data.Hotel.hotelName) == 0) {
                    count++;
                }
            }
            if(count > 1) {
                printf("You have booked hotel '%s' %d times\n", nodes[i]->data.Hotel.hotelName, count);
                found = 1;
            }
        }
    }
    
    if(!found) {
        printf("No duplicates found (tourist places or hotels)\n");
    }
}

void sortHotels() {
    displayTripList();
    int num;
    printf("Enter trip Number to sort Hotels: ");
    scanf("%d",&num);
    
    tripName* trip = findTrip(num);
    if(!trip) {
        printf("Invalid trip number\n");
        return;
    }
    
    // Collect all hotel nodes
    tripNode* hotels[100];
    int hotelCount = 0;
    collectHotels(trip->root, hotels, &hotelCount);
    
    if(hotelCount == 0) {
        printf("No hotels found in this trip\n");
        return;
    }
    
    // Sort hotels by cost (descending) using bubble sort
    for(int i = 0; i < hotelCount - 1; i++) {
        for(int j = 0; j < hotelCount - i - 1; j++) {
            if(hotels[j]->data.Hotel.cost < hotels[j+1]->data.Hotel.cost) {
                tripNode* temp = hotels[j];
                hotels[j] = hotels[j+1];
                hotels[j+1] = temp;
            }
        }
    }
    
    printf("\nHotels sorted by cost (highest to lowest):\n");
    printf("----------------------------------------\n");
    for(int i = 0; i < hotelCount; i++) {
        printf("%d. %s - Cost: %d\n", i+1, hotels[i]->data.Hotel.hotelName, hotels[i]->data.Hotel.cost);
    }
    printf("----------------------------------------\n");
}

char* getLocation(tripNode* t , int d) {
    switch (t->type) {
        case 1: return (d == 1) ? t->data.Flight.From : t->data.Flight.To;
        case 2: return t->data.Hotel.hotelName;
        case 3: return t->data.Tourist.place;
        case 4: return (d == 1) ? t->data.Transport.from : t->data.Transport.to;
        default: return "";
    }
}

void searchNavigation() {
    displayTripList();

    int num;
    printf("Enter trip Number: ");
    scanf("%d", &num);

    tripName* trip = findTrip(num);
    if(!trip){
        printf("Invalid trip\n");
        return;
    }

    if(trip->root == NULL) {
        printf("No activities in this trip\n");
        return;
    }

    char source[50], destination[50];

    printf("Enter Source: ");
    scanf("%s", source);

    printf("Enter Destination: ");
    scanf("%s", destination);

    // Get inorder list of activities
    tripNode* inorderList[100];
    int nodeCount_local = 0;
    getInorderList(trip->root, inorderList, &nodeCount_local);
    
    tripNode* sourceNode = NULL;
    for(int i = 0; i < nodeCount_local; i++) {
        if (strcmp(getLocation(inorderList[i],1), source) == 0) {
            sourceNode = inorderList[i];
            break;
        }
    }

    if (!sourceNode) {
        printf("Source '%s' not found!\n", source);
        return;
    }

    // Find source index
    int sourceIndex = -1;
    for(int i = 0; i < nodeCount_local; i++) {
        if(inorderList[i] == sourceNode) {
            sourceIndex = i;
            break;
        }
    }
    
    tripNode* destNode = NULL;
    for(int i = sourceIndex; i < nodeCount_local; i++) {
        if (strcmp(getLocation(inorderList[i],2), destination) == 0) {
            destNode = inorderList[i];
            break;
        }
    }

    if (!destNode) {
        printf("No route found from %s to %s\n", source, destination);
        return;
    }

    printf("\n Route from %s to %s \n", source, destination);
    printf("\nPath:\n");
    
    int step = 1;
    int destIndex = -1;
    for(int i = 0; i < nodeCount_local; i++) {
        if(inorderList[i] == destNode) {
            destIndex = i;
            break;
        }
    }

    printf("\nStep %d: Start at %s\n", step++, getLocation(sourceNode,1));

    for(int i = sourceIndex; i <= destIndex; i++) {
        tripNode* current = inorderList[i];
        
        switch (current->type) {
            case 1:
                printf("Take flight from %s to %s\n", current->data.Flight.From, current->data.Flight.To);
                break;
            case 2:
                printf("Stay at %s hotel\n", current->data.Hotel.hotelName);
                break;
            case 3:
                printf("Visit tourist place: %s\n", current->data.Tourist.place);
                break;
            case 4:
                printf("Take %s from %s to %s\n", current->data.Transport.mode, 
                       current->data.Transport.from, current->data.Transport.to);
                break;
        }
        
        if (current->navHead) {
            printf("Navigation directions: ");
            displayNav(current->navHead);
        }
        
        printf("Arrive at: %s\n", getLocation(current,2));

        if (i < destIndex) {
            printf("\nStep %d: Continue from %s\n", step++, getLocation(inorderList[i+1],1));
        }
    }
    printf("\n=== Destination Reached! ===\n");
}

void searchNavigationInstruction() {
    displayTripList();

    int num;
    printf("Enter trip Number: ");
    scanf("%d", &num);

    tripName* trip = findTrip(num);
    if(!trip){
        printf("Invalid trip\n");
        return;
    }

    if(trip->root == NULL) {
        printf("No activities in this trip\n");
        return;
    }
    
    displayTrip(trip->root);
    int actId;
    printf("\nEnter activity number to get navigation instructions: ");
    scanf("%d", &actId);
    
    tripNode* temp = searchActivity(trip->root, actId);
    
    if(!temp) {
        printf("Activity number %d not found\n", actId);
        return;
    }
    
    if(!temp->navHead) {
        printf("No navigation instructions for this activity\n");
        return;
    }
    
    char instruction[20];
    printf("Enter navigation direction to search for (e.g., Straight, Left, Right): ");
    scanf("%s", instruction);
    
    navNode* nav = temp->navHead;
    while(nav) {
        if(strcmp(nav->Direction, instruction) == 0) {
            printf("\nFound: %s -> %.1f km\n", nav->Direction, nav->Distance);
            return;
        }
        nav = nav->next;
    }
    printf("Instruction '%s' not found in this activity\n", instruction);
}

void initData() {
    // Create first trip
    tripName* trip1 = (tripName*)malloc(sizeof(tripName));
    strcpy(trip1->name, "Goa Trip");
    trip1->root = NULL;
    trip1->next = tripList;
    tripList = trip1;
    tripCount++;
    
    // Create activities
    tripNode* t1 = createActivityNode();
    strcpy(t1->date, "01-01-2024");
    t1->type = 4;
    t1->time = 9.00;
    strcpy(t1->data.Transport.mode, "Car");
    strcpy(t1->data.Transport.from, "Home");
    strcpy(t1->data.Transport.to, "Airport");
    t1->navHead = createNav("Straight", 5);
    t1->navHead->next = createNav("Left", 2); 
    t1->navHead->next->next = createNav("Right", 3);
    trip1->root = insertActivityAVL(trip1->root, t1);

    tripNode* t2 = createActivityNode();
    strcpy(t2->date, "02-01-2024");
    t2->type = 1;
    t2->time = 14.30;
    strcpy(t2->data.Flight.From, "Hyderabad");
    strcpy(t2->data.Flight.To, "Goa");
    trip1->root = insertActivityAVL(trip1->root, t2);
    
    tripNode* t3 = createActivityNode();
    strcpy(t3->date, "03-01-2024");
    t3->type = 4;
    t3->time = 16.00;
    strcpy(t3->data.Transport.mode, "Car");
    strcpy(t3->data.Transport.from, "Airport");
    strcpy(t3->data.Transport.to, "GoaHotel");
    t3->navHead = createNav("Straight", 5);
    t3->navHead->next = createNav("Left", 2);
    t3->navHead->next->next = createNav("Right", 3);
    trip1->root = insertActivityAVL(trip1->root, t3);

    tripNode* t4 = createActivityNode();
    strcpy(t4->date, "04-01-2024");
    t4->type = 2;
    t4->time = 18.00;
    strcpy(t4->data.Hotel.hotelName, "BeachResort");
    t4->data.Hotel.cost = 5000;
    t4->navHead = createNav("Straight", 1);
    t4->navHead->next = createNav("Right", 2);  
    t4->navHead->next->next = createNav("Left", 1);
    trip1->root = insertActivityAVL(trip1->root, t4);

    tripNode* t5 = createActivityNode();
    strcpy(t5->date, "05-01-2024");
    t5->type = 3;
    t5->time = 10.00;
    strcpy(t5->data.Tourist.place, "BagaBeach");
    trip1->root = insertActivityAVL(trip1->root, t5);

    tripNode* t6 = createActivityNode();
    strcpy(t6->date, "05-01-2024");
    t6->type = 4;
    t6->time = 12.00;
    strcpy(t6->data.Transport.mode, "Car");
    strcpy(t6->data.Transport.from, "BagaBeach");
    strcpy(t6->data.Transport.to, "GoaHotel");
    t6->navHead = createNav("Straight", 1);
    t6->navHead->next = createNav("Right", 2);
    t6->navHead->next->next = createNav("Left", 1);
    trip1->root = insertActivityAVL(trip1->root, t6);

    tripNode* t7 = createActivityNode();
    strcpy(t7->date, "06-01-2024");
    t7->type = 2;
    t7->time = 18.00;
    strcpy(t7->data.Hotel.hotelName, "CityHotel");
    t7->data.Hotel.cost = 3000;
    t7->navHead = createNav("Straight", 1);
    t7->navHead->next = createNav("Right", 2);
    t7->navHead->next->next = createNav("Left", 1);
    trip1->root = insertActivityAVL(trip1->root, t7);

    tripNode* t8 = createActivityNode();
    strcpy(t8->date, "07-01-2024");
    t8->type = 3;
    t8->time = 10.00;
    strcpy(t8->data.Tourist.place, "BagaBeach");
    t8->navHead = createNav("Straight", 1);
    t8->navHead->next = createNav("Right", 2);
    t8->navHead->next->next = createNav("Left", 1);
    trip1->root = insertActivityAVL(trip1->root, t8);

    tripNode* t9 = createActivityNode();
    strcpy(t9->date, "08-01-2024");
    t9->type = 2;
    t9->time = 18.00;
    strcpy(t9->data.Hotel.hotelName, "BudgetInn");
    t9->data.Hotel.cost = 2000;
    trip1->root = insertActivityAVL(trip1->root, t9);
    
    updateActivityIds(trip1->root);
}

void editTrip() {
    displayTripList();
    printf("Enter trip number to edit: ");
    int num;
    scanf("%d", &num);
    tripName* trip = findTrip(num);
    
    if(!trip) {
        printf("Invalid trip number\n");
        return;
    }
    
    printf("\n--- Editing Trip: %s ---\n", trip->name);
    printf("1. Add Activity\n2. Delete Activity\n3. Edit Navigation\n4. Delete Trip\n5. Back\n");
    printf("Enter choice: ");
    int choice;
    scanf("%d", &choice);
    
    switch(choice) {
        case 1: insertActivity(&trip->root); break;
        case 2: if(trip->root == NULL) {
                    printf("No activities in this trip\n");
                    break;
                }
                displayTrip(trip->root);
                printf("Enter activity number to delete: ");
                int activityId;
                scanf("%d", &activityId);
                deleteActivity(&trip->root, activityId); 
                break;
        case 3: if(trip->root == NULL) {
                    printf("No activities in this trip\n");
                    break;
                }
                displayTrip(trip->root);
                printf("Enter activity number to edit navigation: ");
                int navActivityId;
                scanf("%d", &navActivityId);
                tripNode* t = searchActivity(trip->root, navActivityId);
                if(t) {
                    while(1) {
                        printf("\n--- Navigation Menu ---\n");
                        printf("1. Add Nav (at end)\n");
                        printf("2. Insert Nav (at position)\n");
                        printf("3. Delete Nav\n");
                        printf("4. Update Nav\n");
                        printf("5. Back\n");
                        printf("Enter choice: ");
                        int navChoice;
                        scanf("%d", &navChoice);
                        switch(navChoice) {
                            case 1: addNav(&t->navHead); break;
                            case 2: printf("Position to insert: ");
                                    int pos;
                                    scanf("%d", &pos);
                                    insertNav(&t->navHead, pos); break;
                            case 3: printf("Position to delete: ");
                                    int delPos;
                                    scanf("%d", &delPos);
                                    deleteNav(&t->navHead, delPos); break;
                            case 4: printf("Position to update: ");
                                    int updatePos;
                                    scanf("%d", &updatePos);
                                    updateNav(&t->navHead, updatePos); break;
                            case 5: return;
                            default: printf("Invalid choice\n"); 
                        }
                    }
                } else {
                    printf("Activity with ID %d not found\n", navActivityId);
                }
                break;
        case 4: // Delete trip
            {
                tripName* current = tripList;
                tripName* prev = NULL;
                while(current && current != trip) {
                    prev = current;
                    current = current->next;
                }
                if(prev) {
                    prev->next = current->next;
                } else {
                    tripList = current->next;
                }
                freeActivityTree(trip->root);
                free(trip);
                tripCount--;
                printf("Trip deleted successfully!\n");
            }
            break;
        case 5: return;
        default: printf("Invalid choice\n");
    }
}

int main() {
    initData();
    
    while(1) {
        printf("\n========== MAIN MENU ==========\n");
        printf("1. Create Trip\n");
        printf("2. Display Trip\n");
        printf("3. Display All Trips\n");
        printf("4. Edit Trip\n");
        printf("5. Detect Duplicates\n");
        printf("6. Sort Hotels\n");
        printf("7. Search Navigation\n");
        printf("8. Search Navigation by Direction\n");
        printf("9. Exit\n");
        printf("===============================\n");
        printf("Enter your choice: ");
        
        int choice;
        scanf("%d", &choice);
        
        switch(choice) {
            case 1: createTrip(); break;
            case 2: displayTripList();
                    printf("Enter trip number to display: ");
                    int num;
                    scanf("%d", &num);
                    tripName* trip = findTrip(num);
                    if(!trip) {
                        printf("Invalid trip number\n");
                    } else {
                        printf("\n========== Trip Name: %s ==========\n", trip->name);
                        displayTrip(trip->root);
                        printf("===================================\n");
                    }
                    break;
            case 3: {
                        tripName* current = tripList;
                        while(current) {
                            printf("\n========== Trip Name: %s ==========\n", current->name);
                            displayTrip(current->root);
                            printf("===================================\n");
                            current = current->next;
                        }
                    }
                    break;
            case 4: editTrip(); break;
            case 5: detectDuplicates(); break;
            case 6: sortHotels(); break;
            case 7: searchNavigation(); break;
            case 8: searchNavigationInstruction(); break;
            case 9: printf("\nThank you for using Travel Trip Planner!\n");
                    exitProgram(); break;
            default: printf("Invalid choice! Please enter 1-9\n");
        }
    }
    return 0;
}