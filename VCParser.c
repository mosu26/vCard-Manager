/***********************************************
 * Name: Mosu Patel
 * StudentID: 1220619
 * Course: CIS 2750 
 * Assignment: 1
 * Date: February 10th 2025
 * Description: This file contains all mandatory functions to be implemented
 ***********************************************/

#define NUM_VALID_PROPS 38  // Define the number of valid properties
#include "VCParser.h"
#include "VCHelpers.h"


 
VCardErrorCode createCard(char* fileName, Card** newCardObject) {
    if (fileName == NULL || newCardObject == NULL) {
        return INV_FILE;
    }

    // Ensure the file has a .vcf or .vcard extension
    if (!(strstr(fileName, ".vcf") || strstr(fileName, ".vcard"))) {
        return INV_FILE;
    }

    FILE *fp = fopen(fileName, "r");
    if (fp == NULL) {
        return INV_FILE;
    }

    *newCardObject = malloc(sizeof(Card));
    if (*newCardObject == NULL) {
        fclose(fp);
        return INV_FILE;
    }

    // Ensure uninitialized pointers are NULL to prevent undefined behavior
    (*newCardObject)->birthday = NULL;
    (*newCardObject)->anniversary = NULL;
    (*newCardObject)->optionalProperties = initializeList(&propertyToString, &deleteProperty, &compareProperties);
    (*newCardObject)->fn = NULL;

    char line[1024];
    char buffer[1024] = {'\0'};
    int hasBeginTag = 0; // Check if BEGIN:VCARD exists
    int hasEndTag = 0; // checks if END:VCARD exists
    int hasVersion = 0; // check if VERSION 4.0 exists
    int isCRLF = 0; // the flag to help detect the CRLF format

    bool folding = false;

    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);

        // Check for CRLF (`\r\n`)
        if (len >= 2 && line[len - 2] == '\r' && line[len - 1] == '\n') {
            isCRLF = 1;  // CRLF detected sets flag to 1
        }

        line[strcspn(line, "\r\n")] = '\0';  // removes the newline characters
        
        // Check for BEGIN.VCARD
        if (strcasecmp(line, "BEGIN:VCARD") == 0) {
            hasBeginTag = 1;
            continue; 
        }

        // Check for VERSION line
        if (strncasecmp(line, "VERSION:", 8) == 0) { // checks if version is there
            if (strncasecmp(line + 8, "4.0", 3) != 0) { // checks if version is 4.0
                fclose(fp);
                //freeCardOnError(*newCardObject);
                deleteCard(*newCardObject);
                newCardObject = NULL;
                return INV_CARD;
            }
            hasVersion = 1;
            continue;
        }

        // Check for END:VCARD
        if (strcasecmp(line, "END:VCARD") == 0) {
            hasEndTag = 1;
            continue; 
        }

        // Check for invalid line endings (non-CRLF)
        if (!isCRLF) {
            fclose(fp);
            deleteCard(*newCardObject);
            *newCardObject = NULL;
            return INV_CARD;  // Expected error for non-CRLF endings
        }


        // Handle Multi-Line Properties (e.g., ADR, KEY)
        if (line[0] == ' ' || line[0] == '\t') { //if (strpbrk(line, " \t") == line && lastProp != NULL) {  
            if (folding)
                strcat(buffer, line + 1);
            else{
                fclose(fp);
                deleteCard(*newCardObject);
                return INV_CARD;
            }
            continue;  // Move to next line
        }

        if (folding){
            VCardErrorCode e = parseLine(newCardObject, buffer);
            if (e != OK){
                fclose(fp);
                deleteCard(*newCardObject);
                return e;
            }
            buffer[0] = '\0'; // reset buffer
            folding = false;
        }

        // start new property
        strcpy(buffer, line);
        folding = true;

    }

    // any line leftover
    if (folding){
        VCardErrorCode e = parseLine(newCardObject, buffer);
        if (e != OK){
            fclose(fp);
            deleteCard(*newCardObject);
            return e;
        }
        buffer[0] = '\0'; // reset buffer
        folding = false;
    }

    

     if (!hasBeginTag || !hasVersion || !hasEndTag || (*newCardObject)->fn == NULL) {
        deleteCard(*newCardObject);
        fclose(fp);
        *newCardObject = NULL;
        return INV_CARD;
    }


    fclose(fp); // closes file
    return OK;
}


               

                    
char* cardToString(const Card* obj) {
    if (obj == NULL) {
        char* nullStr = malloc(5);
        if (nullStr == NULL) {
            return NULL;
        }
        strcpy(nullStr, "null");
        return nullStr;
    }

    size_t bufferSize = 1024; // Start with a small buffer, expand as needed
    char* str = malloc(bufferSize);
    if (str == NULL) {
        return NULL; // Memory allocation check
    }

    strcpy(str,"");

    // Print FN (Full Name) safely
    if (obj->fn != NULL && obj->fn->values != NULL) {
        char* fnValue = (char*)getFromFront(obj->fn->values);
        if (fnValue != NULL) {
            strcat(str, "Full Name: ");
            strcat(str, fnValue);
            strcat(str, "\n");
        }
    }


    // Print BDAY if it exists
    if (obj->birthday != NULL) {
        //printf("DEBUG: BDAY stored in object: [%s]\n", obj->birthday->text);
        strcat(str, "BDAY: ");
        if (obj->birthday->date != NULL && strlen(obj->birthday->date) > 0) {
            strcat(str, obj->birthday->date);
        }
        if (obj->birthday->time != NULL && strlen(obj->birthday->time) > 0) {
            strcat(str, "T");
            strcat(str, obj->birthday->time);
        }
        
        if (obj->birthday->text != NULL && strlen(obj->birthday->text) > 0) {
            if (strlen(obj->birthday->date) == 0 && strlen(obj->birthday->time) == 0) {
                strcat(str, obj->birthday->text);  // Just print the text directly (no brackets)
            } else {
                strcat(str, " (");
                strcat(str, obj->birthday->text);
                strcat(str, ")");
            }
        }

        if (obj->birthday->UTC) {
            strcat(str, "Z");
        }
        strcat(str, "\n");
    }

    // Print ANNIVERSARY if it exists
    if (obj->anniversary != NULL) {
        strcat(str, "ANNIVERSARY: ");
        if (obj->anniversary->date && strlen(obj->anniversary->date) > 0) {
            strcat(str, obj->anniversary->date);
        }
        if (obj->anniversary->time && strlen(obj->anniversary->time) > 0) {
            strcat(str, "T");
            strcat(str, obj->anniversary->time);
        }
        if (obj->anniversary->text && strlen(obj->anniversary->text) > 0) {
            strcat(str, " (");
            strcat(str, obj->anniversary->text);
            strcat(str, ")");
        }
        if (obj->anniversary->UTC) {
            strcat(str, "Z");
        }
        strcat(str, "\n");
    }

    // Iterate through optionalProperties
    ListIterator iter = createIterator(obj->optionalProperties);
    void* elem;
    while ((elem = nextElement(&iter)) != NULL) {
        Property* prop = (Property*)elem;

        strcat(str, prop->name);

        // Handle Parameters
        if (prop->parameters != NULL && getLength(prop->parameters) > 0) {
            strcat(str, " (");
            ListIterator paramIter = createIterator(prop->parameters);
            void* paramElem;
            int firstParam = 1;
            while ((paramElem = nextElement(&paramIter)) != NULL) {
                Parameter* param = (Parameter*)paramElem;
                if (!firstParam) strcat(str, "; ");
                strcat(str, param->name);
                strcat(str, "=");
                strcat(str, param->value);
                firstParam = 0;
            }
            strcat(str, ")");
        }

        strcat(str, ": ");

        // Special Handling for ADR (Preserve Structure)
        if (strcmp(prop->name, "ADR") == 0) {
            strcat(str, "\n  "); // Indent for readability
        }

        // Print all values in the property
        ListIterator valIter = createIterator(prop->values);
        void* val;
        int first = 1;
        while ((val = nextElement(&valIter)) != NULL) {
            if (!first) {
                if (strcmp(prop->name, "ADR") == 0) {
                    strcat(str, "; "); //  Use `;` for ADR structured values
                } else {
                    strcat(str, ", "); // Use `,` for other properties
                }
            }
            strcat(str, (char*)val);
            first = 0;
        }

        if (getLength(prop->values) == 0) {  // Handle empty properties
            strcat(str, "(No Value)");
        }

        strcat(str, "\n"); // New line after each property
    }

    return str;
}

void deleteCard(Card* obj) {
    if (obj == NULL) {
        return;
    }


    // Free FN property safely
    if (obj->fn != NULL) {
        //printf("Freeing FN property\n");
        free(obj->fn->name);
        free(obj->fn->group);
        freeList(obj->fn->values);
        freeList(obj->fn->parameters); // added this new line...
        free(obj->fn);
    }

    // Free all optional properties safely
    if (obj->optionalProperties != NULL) {
        //printf("Freeing optional properties\n");
        freeList(obj->optionalProperties);  // Now it's safe to free the list
        obj->optionalProperties = NULL;  // Prevents double freeing and use-after-free errors

    }

    // Free birthday if allocated
    if (obj->birthday != NULL) {
        free(obj->birthday->date);
        free(obj->birthday->time);
        free(obj->birthday->text);
        free(obj->birthday);
        obj->birthday = NULL; // prevents a double free
    }

    // Free anniversary if allocated
    if (obj->anniversary != NULL) {
        free(obj->anniversary->date);
        free(obj->anniversary->time);
        free(obj->anniversary->text);
        free(obj->anniversary);
        obj->anniversary = NULL; // prevents a double free
    }

    
    free(obj); // free the card struct
    //printf("Card successfully deleted\n");

}

 

char* errorToString(VCardErrorCode err) {
    char* errorStr = malloc(50 * sizeof(char));  // Allocate enough space for the message
    if (errorStr == NULL) {
        char * defaultErr = malloc(8);
        strcpy(defaultErr,"UNKNOWN");
        return defaultErr;  // Return NULL if memory allocation fails
    }

    switch (err) {
        case OK:
            strcpy(errorStr, "OK");
            break;
        case INV_FILE:
            strcpy(errorStr, "Invalid file");
            break;
        case INV_CARD:
            strcpy(errorStr, "Invalid card");
            break;
        case INV_PROP:
            strcpy(errorStr, "Invalid property");
            break;
        case INV_DT:
            strcpy(errorStr, "Invalid date-time format");
            break;
        default:
            strcpy(errorStr, "Unknown error");
            break;
    }

    return errorStr;
}

// Helper functions for linked lists
int compareValues(const void* first, const void* second) {
    return strcmp((char*)first, (char*)second);
}

void deleteValue(void* toBeDeleted) {

    if (toBeDeleted == NULL) {
        return;
    } 

    char* val = (char*)toBeDeleted;    
    //printf("Freeing value at %p: %s\n", (void*)val, val);  // Debug print
    free(val);
    //toBeDeleted = NULL; // prevents double free
}

char* valueToString(void* value) {
    char* str = malloc(strlen((char*)value) + 1);
    strcpy(str, (char*)value);
    return str;
}

void deleteProperty(void* toBeDeleted) {
    if (toBeDeleted == NULL){
        return;
    } 

    Property* prop = (Property*)toBeDeleted;
    
    free(prop->name);
    prop->name = NULL;
    free(prop->group);
    prop->group = NULL;

    freeList(prop->values);
    freeList(prop->parameters);
    free(prop);
}



char* propertyToString(void* property) {
    Property* prop = (Property*)property;
    char* str = malloc(100);
    sprintf(str, "%s: %s", prop->name, (char*)getFromFront(prop->values));
    return str;
}

int compareProperties(const void* first, const void* second) {
    const Property* prop1 = (const Property*)first;
    const Property* prop2 = (const Property*)second;

    // Compare group names first
    int groupComp = strcmp(prop1->group, prop2->group);
    if (groupComp != 0) {
        return groupComp; // If groups are different, return the comparison result
    }

    // If groups are the same, compare property names
    return strcmp(prop1->name, prop2->name);
}


void deleteParameter(void* toBeDeleted) {
    if (toBeDeleted == NULL) {
        return;
    } 
    Parameter* param = (Parameter*)toBeDeleted;
    
    //printf("Freeing parameter: %s\n", param->name ? param->name : "(null)");

    if (param->name) {
        free(param->name);
    }
    if (param->value) {
        free(param->value);
    }
    
    free(param);
    //param = NULL;
    //printf("Parameter freed successfully\n");

}



int compareParameters(const void* first, const void* second) {
    return strcmp(((Parameter*)first)->name, ((Parameter*)second)->name);
}

char* parameterToString(void* param) {
    Parameter* p = (Parameter*)param;
    char* str = malloc(strlen(p->name) + strlen(p->value) + 3);
    sprintf(str, "%s: %s", p->name, p->value);
    return str;
}

void deleteDate(void* toBeDeleted) {
    DateTime* dt = (DateTime*)toBeDeleted;
    free(dt->date);
    free(dt->time);
    free(dt->text);
    free(dt);
}

int compareDates(const void* first, const void* second) {
    return 0;  // Stub for now
}

char* dateToString(void* date) {
    DateTime* dt = (DateTime*)date;
    char* str = malloc(100);
    sprintf(str, "Date: %s Time: %s Text: %s", dt->date, dt->time, dt->text);
    return str;
}

VCardErrorCode writeCard(const char* fileName, const Card* obj) {

    if (fileName == NULL || obj == NULL || obj->fn == NULL) {
        return WRITE_ERROR;
    }

    if (!(strstr(fileName, ".vcf") || strstr(fileName, ".vcard"))) {
        return WRITE_ERROR;
    }

    FILE *fp = fopen(fileName, "w");
    if (fp == NULL) {
        return WRITE_ERROR;
    }

    fprintf(fp, "BEGIN:VCARD\r\n");
    fprintf(fp, "VERSION:4.0\r\n");


    // Step 5: Write the FN (Full Name) property
    if (obj->fn != NULL && obj->fn->values != NULL) {
        char* fnValue = (char*)getFromFront(obj->fn->values);
        if (fnValue != NULL && strlen(fnValue) > 0) {
            fprintf(fp, "FN:%s\r\n", fnValue);
        }
    }
    
    // Write the N property correctly (ensuring exactly 5 fields)
    Property* nProp = NULL;
    ListIterator iter = createIterator(obj->optionalProperties);
    void* elem;
    while ((elem = nextElement(&iter)) != NULL) {
        Property* prop = (Property*)elem;
        if (strcasecmp(prop->name, "N") == 0) {
            nProp = prop;
            break;
        }
    }


    if (nProp != NULL) {
        fprintf(fp, "N:");
        char* nFields[5] = { "", "", "", "", "" };
        int count = 0;

        ListIterator valIter = createIterator(nProp->values);
        void* val;
        while ((val = nextElement(&valIter)) != NULL && count < 5) {
            //if (strlen((char*)val) > 0) {  // Only store non-empty values in the correct index
            nFields[count] = (char*)val;
            //}
            count++;
        }

        for (int i = 0; i < 5; i++) {
            if (i > 0) fprintf(fp, ";");
            fprintf(fp, "%s", nFields[i]);
        }
        fprintf(fp, "\r\n");
    }

    // Write BDAY if present
    if (obj->birthday != NULL) {
        fprintf(fp, "BDAY:");
        if (obj->birthday->isText) {
            fprintf(fp, "%s", obj->birthday->text);
        } else {
            fprintf(fp, "%s", obj->birthday->date);
            if (strlen(obj->birthday->time) > 0) {
                fprintf(fp, "T%s", obj->birthday->time);
            }
            if (obj->birthday->UTC && obj->birthday->time[strlen(obj->birthday->time) - 1] != 'Z') {
                fprintf(fp, "Z");
            }
        }
        fprintf(fp, "\r\n");
    }

    // Write ANNIVERSARY if present
    if (obj->anniversary != NULL) {
        fprintf(fp, "ANNIVERSARY:");
        if (obj->anniversary->isText) {
            fprintf(fp, "%s", obj->anniversary->text);
        } else {
            fprintf(fp, "%s", obj->anniversary->date);
            if (strlen(obj->anniversary->time) > 0) {
                fprintf(fp, "T%s", obj->anniversary->time);
            }
            if (obj->anniversary->UTC && obj->anniversary->time[strlen(obj->anniversary->time) - 1] != 'Z') {
                fprintf(fp, "Z");
            }
        }
        fprintf(fp, "\r\n");
    }

    // Write remaining properties in correct order
    iter = createIterator(obj->optionalProperties);
    while ((elem = nextElement(&iter)) != NULL) {
        Property* prop = (Property*)elem;
        if (strcasecmp(prop->name, "N") == 0) {
            continue; // Skip N because we already wrote it
        }

        // Print the group name if it exists
        if (strlen(prop->group) > 0) {
            fprintf(fp, "%s.", prop->group); // Print group name followed by a dot
        }

        fprintf(fp, "%s", prop->name);

        if (getLength(prop->parameters) > 0) {
            ListIterator paramIter = createIterator(prop->parameters);
            void* paramElem;
            while ((paramElem = nextElement(&paramIter)) != NULL) {
                Parameter* param = (Parameter*)paramElem;
                fprintf(fp, ";%s=%s", param->name, param->value);
            }
        }


        // Special case: ADR property
        if (strcasecmp(prop->name, "ADR") == 0) {
            //printf("DEBUG: Writing ADR property:\n");

            fprintf(fp, ":");

            // Ensure exactly 7 fields
            char* adrFields[7] = { "", "", "", "", "", "", "" };
            int count = 0;

            ListIterator adrIter = createIterator(prop->values);
            void* adrVal;
            while ((adrVal = nextElement(&adrIter)) != NULL && count < 7) {
                adrFields[count] = (char*)adrVal;
                count++;
            }

            // Print ADR fields, ensuring exactly 7 fields with `;` delimiters
            for (int i = 0; i < 7; i++) {
                if (i > 0) fprintf(fp, ";");
                fprintf(fp, "%s", adrFields[i]);  // Print field (empty fields preserved)
            }
            fprintf(fp, "\r\n");
            continue;
        }


        if (prop->values != NULL && getLength(prop->values) > 0) {
            fprintf(fp, ":");
            ListIterator valIter = createIterator(prop->values);
            void* val;
            int first = 1;
            while ((val = nextElement(&valIter)) != NULL) {
                if (!first) fprintf(fp, ";");
                fprintf(fp, "%s", (char*)val);
                first = 0;
            }
        }
        fprintf(fp, "\r\n");
    }

    fprintf(fp, "END:VCARD\r\n");
    fclose(fp);
    return OK;
}


VCardErrorCode validateCard(const Card* obj) {

    // Step 1: Check if the Card object is NULL
    if (obj == NULL) {
        return INV_CARD;
    }
    // Defining all valid properties 
    const char* validProps[NUM_VALID_PROPS] = {"BEGIN","END","VERSION",
        "SOURCE", "KIND", "XML", "FN", "N", "NICKNAME", "PHOTO", "BDAY", "ANNIVERSARY",
        "GENDER", "ADR", "TEL", "EMAIL", "IMPP", "LANG", "TZ", "GEO", "TITLE", "ROLE",
        "LOGO", "ORG", "MEMBER", "RELATED", "CATEGORIES", "NOTE", "PRODID", "REV",
        "SOUND", "UID", "CLIENTPIDMAP", "URL", "KEY", "FBURL",
        "CALADRURI", "CALURI"
    };

    // Validate FN (Full Name) - Required Property
    if (obj->fn == NULL || getLength(obj->fn->values) != 1) {
        return INV_CARD;  // FN is mandatory and must be present and must have exactly one value
    }
    
    // FN must have at least one valid text value
    char* fnValue = (char*)getFromFront(obj->fn->values);
    if (fnValue == NULL || strlen(fnValue) == 0) {
        return INV_CARD;  // FN cannot be empty
    }


    // Check if optionalProperties is NULL before iterating
    if (obj->optionalProperties == NULL) {
        return INV_CARD;  // Missing optionalProperties is an invalid card
    }

    // Validate BDAY and ANNIVERSARY (Must NOT be in optionalProperties)
    if (obj->birthday != NULL) {
        DateTime* bday = obj->birthday;

        // Ensure internal consistency
        if (bday->isText) {
            // Text-based DateTime must have empty date/time and cannot be UTC
            if (strlen(bday->date) > 0 || strlen(bday->time) > 0 || bday->UTC) {
                return INV_DT;
            }
        } else {
            // If it's NOT a text DateTime, at least date OR time must exist
            if (strlen(bday->date) == 0 && strlen(bday->time) == 0) {
                return INV_DT;  // Date must exist
            }

            // Ensure non-text DateTime does not contain a text field
            if (strlen(bday->text) > 0) {
                return INV_DT;
            }
        }
    }

    if (obj->anniversary != NULL) {
        DateTime* anniversary = obj->anniversary;

        // Ensure internal consistency
        if (anniversary->isText) {
            if (strlen(anniversary->date) > 0 || strlen(anniversary->time) > 0 || anniversary->UTC) {
                return INV_DT;
            }
        } else {
            // If it's NOT a text DateTime, at least date OR time must exist
            if (strlen(anniversary->date) == 0 && strlen(anniversary->time) == 0) { 
                return INV_DT;
            }

            // Ensure non-text DateTime does not contain a text field
            if (strlen(anniversary->text) > 0) {
                return INV_DT;
            }
        }
    }


    // Step 2: Iterate through optionalProperties to find and validate 
    ListIterator iter = createIterator(obj->optionalProperties);
    void* elem;
    int kindCount = 0;
    int nCount = 0;
    int bdayCount = 0;
    int annivCount = 0;
    int prodIDCount = 0;
    int revCount = 0;
    int genderCount = 0;
    int uidCount = 0;
    

    if (obj->birthday != NULL) {
        bdayCount = 1; // if a birthday is already set, start at 1
    }

    if (obj->anniversary != NULL) {
        annivCount = 1; // if an anniversary is already set, start at 1
    }
    Property* nProp = NULL;

    while ((elem = nextElement(&iter)) != NULL) {
        Property* prop = (Property*)elem;
        // Ensure the property is from Sections 6.1 - 6.9.3
        int isValid = 0; // reset the isValid for each property
        for (int i = 0; i < NUM_VALID_PROPS; i++) {  // Use the predefined constant
            if (strcasecmp(prop->name, validProps[i]) == 0) {
                isValid = 1;  // Found a match, so it's a valid property
                break;
            }
        }
        
        // If the property is not in the valid list, return an error
        if (!isValid) {
            return INV_PROP;  // Property is not recognized, so it's invalid
        }
        
  
        // Ensure BEGIN is NOT in optionalProperties
        if (strcasecmp(prop->name, "BEGIN") == 0) {
            return INV_PROP; 
        }

         // Ensure END is NOT in optionalProperties
         if (strcasecmp(prop->name, "END") == 0) {
            return INV_PROP; 
        }

        // Ensure VERSION is NOT in optionalProperties
        if (strcasecmp(prop->name, "VERSION") == 0) {
            return INV_CARD;  // VERSION must NOT be in optionalProperties
        }

        // Ensure BDAY and ANNIVERSARY are NOT in optionalProperties
        if (strcasecmp(prop->name, "BDAY") == 0) {
            bdayCount++;
            if (bdayCount > 1) {
                return INV_DT;  // Multiple BDAY properties are not allowed
            }
        }

        if (strcasecmp(prop->name, "ANNIVERSARY") == 0) {
            annivCount++;
            if (annivCount > 1) {
                return INV_DT;  // Multiple ANNIVERSARY properties are not allowed
            }
        }

        // Check if property has at least one value
        if (getLength(prop->values) == 0) {
            return INV_PROP;  // Properties must have at least one value
        }

        // Skip empty value check for N and ADR since they allow empty fields
        if (strcasecmp(prop->name, "N") != 0 && strcasecmp(prop->name, "ADR") != 0) {

            // Check if any property value is empty
            ListIterator valIter = createIterator(prop->values);
            void* val;
            while ((val = nextElement(&valIter)) != NULL) {
                char* propValue = (char*)val;
                
                if (strlen(propValue) == 0) {
                    return INV_PROP;  // Property cannot have an empty value
                }
            }
        }

        // Ensure all parameters have both a name and a value
        ListIterator paramIter = createIterator(prop->parameters);
        void* paramElem;
        while ((paramElem = nextElement(&paramIter)) != NULL) {
            Parameter* param = (Parameter*)paramElem;

            // Check if parameter name is missing or empty
            if (param->name == NULL || strlen(param->name) == 0) {
                return INV_PROP; // Parameter name must exist
            }

            // Check if parameter value is missing or empty
            if (param->value == NULL || strlen(param->value) == 0) {
                return INV_PROP; // Parameter value must exist
            }
        }

        // Validate the N (Name) property (if it exists)
        if (strcasecmp(prop->name, "N") == 0) {
            nCount++;
            nProp = prop;

            // Ensure there is at most ONE `N` property
            if (nCount > 1) {
                return INV_PROP;
            }

            // Ensure it contains exactly 5 fields
            if (getLength(nProp->values) != 5) {
                return INV_PROP;  // Must have exactly 5 fields
            }

            // Check that each field exists (empty is allowed, missing is not)
            ListIterator nIter = createIterator(nProp->values);
            void* nValue;
            int fieldCount = 0;

            while ((nValue = nextElement(&nIter)) != NULL) {
                char* field = (char*)nValue;

                // If a field is NULL or missing completely (e.g. only 3 fields exist are there out of the 5)
                if (field == NULL) {
                    return INV_PROP;
                }

                fieldCount++;
            }

            // Ensure exactly 5 fields were iterated over
            if (fieldCount != 5) {
                return INV_PROP;
            }
        }


        if (strcasecmp(prop->name, "ADR") == 0) {
            // Ensure ADR contains exactly 7 fields
            if (getLength(prop->values) != 7) {
                return INV_PROP;  // Must have exactly 7 fields
            }
        
            // Check that each field exists (empty is allowed, NULL is not)
            ListIterator adrIter = createIterator(prop->values);
            void* adrValue;
            int afieldCount = 0;
        
            while ((adrValue = nextElement(&adrIter)) != NULL) {
                char* field = (char*)adrValue;
        
                // Ensure no field is NULL (empty is okay)
                if (field == NULL) {
                    return INV_PROP;
                }
        
                afieldCount++;
            }
        
            // Ensure exactly 7 fields were iterated over
            if (afieldCount != 7) {
                return INV_PROP;
            }
        }

        // Validate the NICKNAME property if it exists
        if (strcasecmp(prop->name, "NICKNAME") == 0) {
            ListIterator nickIter = createIterator(prop->values);
            void *nickVal;
            

            while((nickVal = nextElement(&nickIter)) != NULL) {
                char* nickname = (char*)nickVal;
                // Ensures that the value is valid 
                if (strchr(nickname, ',') != NULL) { // multiple nicknames are comma-seperated 
                    continue;
                }
            }
        }

        // Check if the property is SOURCE
        if (strcasecmp(prop->name, "SOURCE") == 0) {
            
            // SOURCE must contain a valid URI - only 1 value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // SOURCE must have one URI value
            }
        }

        // Check if the property is KIND
        if (strcasecmp(prop->name, "KIND") == 0) {
            kindCount++;
            if (kindCount > 1) {
                return INV_PROP;  // Multiple KIND properties are not allowed
            }
            
            // Validate KIND value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // KIND must have exactly one value
            }
            
        }

        // Check if the property is XML
        if (strcasecmp(prop->name, "XML") == 0) {
            if (getLength(prop->values) != 1) {
                return INV_PROP; // XML is a must have at most one value
            }
            
        }


        // Validate the PHOTO property (Only checking cardinality)
        if (strcasecmp(prop->name, "PHOTO") == 0) {
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // PHOTO must have at most one value
            }
        }

        // Validate GENDER property
        if (strcasecmp(prop->name, "GENDER") == 0) {
            genderCount++;

            // Ensure there is at most ONE `GENDER` property
            if (genderCount > 1) {
                return INV_PROP;
            }

            // Ensure GENDER has exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;
            }
        }

        // Validate TEL Property
        if (strcasecmp(prop->name, "TEL") == 0) {
            // TEL must have at least one value
            if (getLength(prop->values) == 0) {
                return INV_PROP;
            }

            // Ensure no TEL value is empty
            ListIterator telIter = createIterator(prop->values);
            void* telVal;
            while ((telVal = nextElement(&telIter)) != NULL) {
                char* telNumber = (char*)telVal;

                if (strlen(telNumber) == 0) {
                    return INV_PROP;
                }
            }
        }

        // Validate EMAIL Property
        if (strcasecmp(prop->name, "EMAIL") == 0) {
            // Ensure each EMAIL property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // EMAIL must contain exactly one value
            }
        }


        // Validate IMPP Property (Instant Messaging URI)
        if (strcasecmp(prop->name, "IMPP") == 0) {
            // Ensure each IMPP property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // IMPP must contain exactly one URI value
            }
        }

        // Validate LANG Property
        if (strcasecmp(prop->name, "LANG") == 0) {
            // Ensure each LANG property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // LANG must contain exactly one value
            }
        }

        // Validate TZ Property
        if (strcasecmp(prop->name, "TZ") == 0) {
            // Ensure each TZ property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // TZ must contain exactly one value
            }
        }

        // Validate GEO Property
        if (strcasecmp(prop->name, "GEO") == 0) {
            // Ensure each GEO property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // GEO must contain exactly one value
            }
        }

        // Validate TITLE Property
        if (strcasecmp(prop->name, "TITLE") == 0) {
            // Ensure each TITLE property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // TITLE must contain exactly one value
            }
        }

        // Validate ROLE Property
        if (strcasecmp(prop->name, "ROLE") == 0) {
            // Ensure each ROLE property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // ROLE must contain exactly one value
            }
        }

        // Validate LOGO Property
        if (strcasecmp(prop->name, "LOGO") == 0) {
            // Ensure each LOGO property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // LOGO must contain exactly one value
            }
        }

        // Validate ORG Property
        if (strcasecmp(prop->name, "ORG") == 0) {
            // Ensure each ORG property contains exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // ORG must contain exactly one structured text value
            }
        }

        // Validate MEMBER property
        if (strcasecmp(prop->name, "MEMBER") == 0) {
            // MEMBER must have one value
            if (getLength(prop->values) != 1) {
                return INV_PROP;
            }
        }

        // Validate RELATED property (only checking cardinality)
        if (strcasecmp(prop->name, "RELATED") == 0) {
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // RELATED must have one value
            }
        }

        // Validate CATEGORIES property (only checking cardinality)
        if (strcasecmp(prop->name, "CATEGORIES") == 0) {
            if (getLength(prop->values) == 0) {
                return INV_PROP;  // CATEGORIES must have at least one value
            }
        }

        // Validate NOTE property (only checking cardinality)
        if (strcasecmp(prop->name, "NOTE") == 0) {
            if (getLength(prop->values) != 1) { // NOTE must have at one value
                return INV_PROP;  
            }
        }

        // Validate PRODID property
        if (strcasecmp(prop->name, "PRODID") == 0) {
            prodIDCount++;

            // Ensure there is at most ONE `PRODID` property
            if (prodIDCount > 1) {
                return INV_PROP;  // More than one PRODID is not allowed
            }

            // Ensure PRODID has exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;
            }
        }

        // Validate REV property
        if (strcasecmp(prop->name, "REV") == 0) {
            revCount++;

            // Ensure there is at most ONE `REV` property
            if (revCount > 1) {
                return INV_PROP;  // More than one REV is not allowed
            }

            // Ensure REV has exactly ONE value
            if (getLength(prop->values) != 1) {
                return INV_PROP;
            }
        }

        // Validate SOUND Property
        if (strcasecmp(prop->name, "SOUND") == 0) {
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // SOUND must have exactly one value
            }
        }

        // Validate UID Property
        if (strcasecmp(prop->name, "UID") == 0) {
            uidCount++;

            // cardinality - only one UID allowed)
            if (uidCount > 1) {
                return INV_PROP;
            }

            // UID must have exactly one value
            if (getLength(prop->values) != 1) {
                return INV_PROP;
            }

        }

        if (strcasecmp(prop->name, "CLIENTPIDMAP") == 0) {
            // Ensure CLIENTPIDMAP has exactly two values (integer ; URI)
            if (getLength(prop->values) != 2) {
                return INV_PROP;
            }
        }

        if (strcasecmp(prop->name, "URL") == 0) {
            // Ensure URL has exactly one value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // URL must have exactly one value
            }
        }

        if (strcasecmp(prop->name, "KEY") == 0) {
            // Ensure KEY has exactly one value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // KEY must have exactly one value
            }
        }

        if (strcasecmp(prop->name, "FBURL") == 0) {
            // Ensure FBURL has exactly one value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // FBURL must have exactly one URI value
            }
        }

        if (strcasecmp(prop->name, "CALADRURI") == 0) {
            // Ensure CALADRURI has exactly one value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // CALADRURI must have exactly one URI value
            }
        }

        if (strcasecmp(prop->name, "CALURI") == 0) {
            // Ensure CALURI has exactly one value
            if (getLength(prop->values) != 1) {
                return INV_PROP;  // CALURI must have exactly one URI value
            }
        }
    }
    return OK;  // return ok is everything is fine

}