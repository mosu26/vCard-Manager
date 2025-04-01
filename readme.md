README - vCard Manager Assignment (CIS*2750)

Author: Mosu Patel
ID: 1220619
Date: Thursday March 27th, 2025


Description:

The vCard Manager is a terminal-based contact management application designed using Python and integrated with a custom C library (libvcparser.so). It allows users to interact with v card files (.vcf), enabling viewing, editing, and creation of new vCards. Users also have the option to connect to a MySQL database for persistent data storage and querying of vCard data.


Features:
- Application asks user to enter credentials to log into database
  - Application will continue to function and allow functionality such as editing, and creating new cards even if the database login is skipped (except DB-related features)
  - New cards/.vcf files will be created and file info that is edited (such as first/contact name) will be reflected in the files themselves, however not in the database as user is not connected

- UI built using asciimatics for a user-friendly terminal interface
- Automatic validation of .vcf files using a C library
- View list of all valid vCards from the cards/ directory
- Detailed view of vCard information including:
  - Contact name
  - Birthday and Anniversary (if present)
-   Number of optional properties

- Create new vCard files with a custom contact name
- Edit the contact name of existing vCards
- Optional database integration for persistent tracking

- Run SQL queries from within the UI:
  - Display all contacts (in order by first name) with associated file metadata
  - Find all contacts born in June, sorted by age


Implementation Details:

- libvcparser.so: C shared library responsible for parsing, validating, and editing vCard files
- A3main.py: Main Python driver that connects the UI, database logic, and C library
- UI built with asciimatics.widgets (e.g., Frame, ListBox, Text, Button, etc.)
- Valid .vcf files are dynamically loaded from the ./cards/ folder
- Validated vCards may optionally be added to a MySQL database if a connection is active
- Birthday and anniversary fields are parsed and converted into Python datetime objects using additional helper functions in C
- Database schema consists of two tables: FILE and CONTACT

Assumptions:

- Timestamps shown from the database (e.g., last_modified, creation_time) are in UTC time. Local timezone offsets are not considered. This applies to both the FILE and CONTACT tables
  - By default, Python’s os.path.getmtime() returns timestamps in your local time zone (for me it's Canadian Eastern Time), the database server or terminal is set to store or display times in UTC
  - UTC is around 4 hours ahead than Canadian Eastern Time, so database will show times which are 4 hours ahead 
- cards/ directory must exist in the same location as the main script (A3main.py) and contain all .vcf files to be processed
- .vcf files that fail validation will not appear in the UI or be added to the database
- The application gracefully handles skipped database login; non-database features remain functional (e.g. edit and create)
- Edited or newly created cards are only added to the database if a DB connection was established during the session

How to Run:

1. Make sure you are in the correct directory and run the make command. This is to make sure `libvcparser.so` is compiled and present in the `bin/` directory.

2. Run the program going into the bin directory and than doing: 
      python3 A3main.py
   