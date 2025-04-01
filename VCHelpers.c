/***********************************************
 * Name: Mosu Patel
 * StudentID: 1220619
 * Course: CIS 2750 
 * Assignment: 1
 * Date: February 10th 2025
 * Description: This file contains my helper functions
 ***********************************************/


#include "VCHelpers.h"


DateTime* parseDateTime(char* dateString) {
    if (dateString == NULL) {
        return NULL;
    }

    char* colon = strchr(dateString, ':');
    if (colon == NULL)
        return NULL;
    else
        dateString = colon + 1;

    //printf("DEBUG: Parsing dateString: %s\n", dateString);  

    DateTime* dateTime = malloc(sizeof(DateTime));
    if (dateTime == NULL) {
        return NULL;
    }

    // Allocate memory for fields (Ensure null termination)
    dateTime->date = calloc(15, sizeof(char));
    dateTime->time = calloc(15, sizeof(char));
    dateTime->text = calloc(strlen(dateString) + 1, sizeof(char));
    dateTime->UTC = false;
    dateTime->isText = false;

    if (!dateTime->date || !dateTime->time || !dateTime->text) {
        free(dateTime->date);
        free(dateTime->time);
        free(dateTime->text);
        free(dateTime);
        return NULL;
    }

    // Initialize empty strings
    dateTime->date[0] = '\0';
    dateTime->time[0] = '\0';
    dateTime->text[0] = '\0';

    //  Step 1: Check if it's a text-based date (contains non-numeric characters except T or Z)
    for (int i = 0; dateString[i] != '\0'; i++) {
        if (!isdigit(dateString[i]) && dateString[i] != 'T' && dateString[i] != 'Z' && dateString[i] != '-') { // ADDED - check for dash and changed isalpha to !isdigit
            strcpy(dateTime->text, dateString);
            dateTime->isText = true;
            return dateTime;
        }
    }

    // Step 2: Check for 'T' (date-time separator)
    char* T_position = strchr(dateString, 'T');
    if (T_position != NULL) {
        // ADDED - put first half in date and second half in time
        strncpy(dateTime->date, dateString, T_position - dateString);
        dateTime->date[T_position - dateString] = '\0';
        strcpy(dateTime->time, T_position + 1);
        int len = strlen(dateTime->time);
        if (dateTime->time[len - 1] == 'Z'){
            dateTime->UTC = true;
            dateTime->time[len - 1] = '\0';
        }
    }
    else{
        strcpy(dateTime->date, dateString); // entire value is a date
    }

    return dateTime;
}

VCardErrorCode parseLine(Card ** newCardObject, char* line){

    if (line == NULL)
        return OK;

    char* colon = strchr(line, ':');

    if (colon == NULL || colon == line)
        return INV_PROP;
    
    Property* p = malloc(sizeof(Property));
    p->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    p->values = initializeList(valueToString, deleteValue, compareValues);

    char* semicolon = strchr(line, ';');
    char* dot = strchr(line, '.');
    char* end = NULL;
    if (semicolon != NULL && semicolon < colon)
        end = semicolon;
    else
        end = colon;
    
    // Parse group
    if (dot != NULL && dot < end){
        int len = dot - line;
        p->group = malloc(sizeof(char) * (len + 1));
        strncpy(p->group, line, len);
        p->group[len] = '\0';

        len = end - (dot + 1);
        p->name = malloc(sizeof(char) * (len + 1));
        strncpy(p->name, dot + 1, len);
        p->name[len] = '\0';
    }
    else{
        int len = end - line;
        p->name = malloc(sizeof(char) * (len + 1));
        strncpy(p->name, line, len);
        p->name[len] = '\0';
        p->group = malloc(sizeof(char));
        p->group[0] = '\0';
    }

    if (strcasecmp(p->name, "BDAY") == 0 || strcasecmp(p->name, "ANNIVERSARY") == 0){
        bool isBday = strcasecmp(p->name, "BDAY") == 0;
        free(p->name);
        free(p->group);
        freeList(p->parameters);
        freeList(p->values);
        free(p);
        DateTime* dt = parseDateTime(line);
        if (dt == NULL)
            return INV_DT;
        else if (isBday)
            (*newCardObject)->birthday = dt;
        else
            (*newCardObject)->anniversary = dt;
        return OK;
    }

    // Parse parameters
    if (semicolon && semicolon < colon){
        char* curr = semicolon + 1;
        while (curr < colon){
            char* equal = strchr(curr, '=');
            if (!equal || equal > colon){
                free(p->name);
                free(p->group);
                freeList(p->parameters);
                freeList(p->values);
                free(p);
                return INV_PROP;
            }
            
            char* next = strchr(curr, ';');
            if (!next || next > colon)
                next = colon;
            
            if (equal == curr || equal + 1 == next){
                free(p->name);
                free(p->group);
                freeList(p->parameters);
                freeList(p->values);
                free(p);
                return INV_PROP;
            }
            
            Parameter* param = malloc(sizeof(Parameter));
            param->name = calloc(50,sizeof(char));// param->name = malloc(sizeof(char) * 50);
            param->value = calloc(50,sizeof(char));//param->value = malloc(sizeof(char) * 50);
            strncpy(param->name, curr, equal - curr);
            strncpy(param->value, equal + 1, next - equal - 1);

            insertBack(p->parameters, param);
            curr = next + 1;
        }
    }

    // Parse values
    char* curr = colon + 1;
    while (curr != NULL && *curr != '\0'){
        char* next = strchr(curr, ';'); // Find next semicolon
        if (next == NULL) { // last value
            break;
        }
        else if (next == curr) { 
            char* value = malloc(sizeof(char));
            value[0] = '\0';
            insertBack(p->values, value);
            curr = curr + 1;
        } 
        else {
            char* value = malloc(sizeof(char) * (next - curr + 1));
            strncpy(value, curr, next - curr);
            value[next - curr] = '\0';
            insertBack(p->values, value);
            curr = next + 1;
        }
    }

    // get final value
    if (curr != NULL){
        char* finalVal = malloc(sizeof(char) * (strlen(curr) + 1));
        strcpy(finalVal, curr);
        insertBack(p->values, finalVal);
    }

    int n = getLength(p->values);

    if (strcasecmp(p->name, "N") == 0 && n != 5){
        free(p->name);
        free(p->group);
        freeList(p->parameters);
        freeList(p->values);
        free(p);
        return INV_PROP;
    }
    else if (strcasecmp(p->name, "ADR") == 0 && n != 7){
        free(p->name);
        free(p->group);
        freeList(p->parameters);
        freeList(p->values);
        free(p);
        return INV_PROP;
    }
    else if (strcasecmp(p->name, "FN") == 0){
        if (n != 1){
            free(p->name);
            free(p->group);
            freeList(p->parameters);
            freeList(p->values);
            free(p);
            return INV_PROP;
        }
        else if ((*newCardObject)->fn == NULL){
            (*newCardObject)->fn = p;
            return OK;
        }
    }

    insertBack((*newCardObject)->optionalProperties, p);

    return OK;
}




// Helper function to allocate a copy of a string
char* allocateString(const char* source) {
    if (source == NULL) {
        return NULL;
    }
    size_t len = strlen(source) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        strcpy(copy, source);
    }
    return copy;
}

// Get Contact Name (FN property)
char* getCardFullName(Card* card) {
    if (card == NULL || card->fn == NULL || getLength(card->fn->values) == 0) {
        return allocateString("Unknown");
    }
    return allocateString((char*)getFromFront(card->fn->values)); // Return first FN value
}

// Get Birthday
char* getCardBirthday(Card* card) {
    if (card == NULL || card->birthday == NULL) {
        return allocateString("");  // Empty string if no birthday
    }

    // If it's a text-type birthday (like "circa 1960"), show that directly
    if (card->birthday->isText && strlen(card->birthday->text) > 0) {
        return allocateString(card->birthday->text);
    }
    
    char buffer[100] = {0}; // Temporary buffer
    if (strlen(card->birthday->date) > 0) {
        strcat(buffer, "Date: ");
        strcat(buffer, card->birthday->date);
    }
    if (strlen(card->birthday->time) > 0) {
        strcat(buffer, " Time: ");
        strcat(buffer, card->birthday->time);
    }
    if (card->birthday->UTC) {
        strcat(buffer, " (UTC)");
    }
    return allocateString(buffer);
}

// Get Anniversary
char* getCardAnniversary(Card* card) {
    if (card == NULL || card->anniversary == NULL) {
        return allocateString("");  // Empty string if no anniversary
    }

    // If it's a text-type anniversary (like "circa 1960"), show that directly
    if (card->anniversary->isText && strlen(card->anniversary->text) > 0) {
        return allocateString(card->anniversary->text);
    }

    char buffer[100] = {0}; // Temporary buffer
    if (strlen(card->anniversary->date) > 0) {
        strcat(buffer, "Date: ");
        strcat(buffer, card->anniversary->date);
    }
    if (strlen(card->anniversary->time) > 0) {
        strcat(buffer, " Time: ");
        strcat(buffer, card->anniversary->time);
    }
    if (card->anniversary->UTC) {
        strcat(buffer, " (UTC)");
    }
    return allocateString(buffer);
}

// Get Other Properties Count
int getOtherPropertiesCount(Card* card) {
    if (card == NULL || card->optionalProperties == NULL) {
        return 0;
    }
    return getLength(card->optionalProperties);
}

VCardErrorCode setCardFullName(Card* card, const char* newFN) {
    if (card == NULL || newFN == NULL || strlen(newFN) == 0) {
        return INV_PROP;
    }

    // If FN property doesn't exist, create it
    if (card->fn == NULL) {
        card->fn = malloc(sizeof(Property));
        if (!card->fn) return OTHER_ERROR;
        
        card->fn->name = malloc(3); // "FN"
        strcpy(card->fn->name, "FN");

        card->fn->group = malloc(1);
        card->fn->group[0] = '\0';  // Empty group

        card->fn->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
        card->fn->values = initializeList(valueToString, deleteValue, compareValues);
    }

    // Free existing FN value if it exists
    if (getLength(card->fn->values) > 0) {
        deleteDataFromList(card->fn->values, getFromFront(card->fn->values));
    }

    // Allocate and copy new FN value
    char* newValue = malloc(strlen(newFN) + 1);
    if (!newValue) {
        return OTHER_ERROR;
    } 
    strcpy(newValue, newFN);

    insertFront(card->fn->values, newValue);

    return OK;  // Return success
}



Card* createEmptyCard() {
    Card* newCard = malloc(sizeof(Card));
    if (!newCard) return NULL;

    newCard->fn = NULL;
    newCard->birthday = NULL;
    newCard->anniversary = NULL;
    newCard->optionalProperties = initializeList(propertyToString, deleteProperty, compareProperties);

    return newCard;
}



char* getCardBirthdayRaw(Card* card) {
    if (card == NULL || card->birthday == NULL || card->birthday->isText) {
        return allocateString("");  // Empty if text or missing
    }

    char buffer[100] = {0};
    if (strlen(card->birthday->date) > 0) {
        strcat(buffer, card->birthday->date);
        if (strlen(card->birthday->time) > 0) {
            strcat(buffer, "T");
            strcat(buffer, card->birthday->time);
        }
    }
    return allocateString(buffer);
}

char* getCardAnniversaryRaw(Card* card) {
    if (card == NULL || card->anniversary == NULL || card->anniversary->isText) {
        return allocateString("");  // Empty if text or missing
    }

    char buffer[100] = {0};
    if (strlen(card->anniversary->date) > 0) {
        strcat(buffer, card->anniversary->date);
        if (strlen(card->anniversary->time) > 0) {
            strcat(buffer, "T");
            strcat(buffer, card->anniversary->time);
        }
    }
    return allocateString(buffer);
}
