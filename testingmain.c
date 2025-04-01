/***********************************************
 * Name: Mosu Patel
 * StudentID: 1220619
 * Course: CIS 2750 
 * Assignment: 1
 * Date: February 10th 2025
 * Description: This file is my main for testing
 ***********************************************/


#include "VCParser.h"



int main() {
    Card* myCard;
    
    // Read a vCard file into a Card object
    VCardErrorCode result = createCard("testCard.vcf", &myCard);

    if (result == OK) {
        printf("Card created successfully!\n");

        // Convert the card to string for debugging
        char* cardStr = cardToString(myCard);
        printf("Card Contents:\n%s\n", cardStr);

        // Test writeCard by writing myCard to a new file
        VCardErrorCode writeResult = writeCard("output.vcf", myCard);
        if (writeResult == OK) {
            printf("Card written successfully to output.vcf!\n");
        } else {
            char *writeErrorStr = errorToString(writeResult);
            printf("Error writing card: %s\n", writeErrorStr);
            free(writeErrorStr);
        }

        // Cleanup
        free(cardStr);
        deleteCard(myCard);
        printf("Card deleted successfully!\n");
    } else {
        char* errorStr = errorToString(result);
        printf("Error: %s\n", errorStr);
        free(errorStr);
    }

    return 0;
}
