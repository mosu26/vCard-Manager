/***********************************************
 * Name: Mosu Patel
 * Date: February 10th 2025
 * Description: This file is the header file for my helper functions and additional libraries
 ***********************************************/

#ifndef _VCHELPERS_H
#define _VCHELPERS_H

#include <ctype.h>
#include <strings.h>
#include "LinkedListAPI.h"
#include "VCParser.h"

DateTime* parseDateTime(char* dateString);
VCardErrorCode parseLine(Card ** newCardObject, char* line);
char* getCardFullName(Card* card);
char* getCardBirthday(Card* card);
char* getCardAnniversary(Card* card);
int getOtherPropertiesCount(Card* card);
VCardErrorCode setCardFullName(Card* card, const char* newFN);
Card* createEmptyCard();
char* getCardAnniversaryRaw(Card* card);
char* getCardBirthdayRaw(Card* card);


#endif
