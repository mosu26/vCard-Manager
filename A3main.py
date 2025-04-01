#!/usr/bin/env python3

import os
import ctypes
from asciimatics.widgets import Frame, ListBox, Layout, Divider, Button, Widget
from asciimatics.widgets import Label
from asciimatics.widgets import Text
from asciimatics.widgets import TextBox
from asciimatics.scene import Scene
from asciimatics.screen import Screen
from asciimatics.exceptions import NextScene, StopApplication
import mysql.connector
import datetime

# Load C library (make sure libvcparser.so is in the bin/ directory)
# lib = ctypes.CDLL("./libvcparser.so")

# Get the absolute path of the `bin/` directory
bin_dir = os.path.dirname(os.path.abspath(__file__))

# Load the shared library using the absolute path
lib_path = os.path.join(bin_dir, "libvcparser.so")
lib = ctypes.CDLL(lib_path)

# Define function prototypes for createCard and validateCard
lib.createCard.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p)]
lib.createCard.restype = ctypes.c_int

lib.validateCard.argtypes = [ctypes.c_void_p]
lib.validateCard.restype = ctypes.c_int

# Define function prototype for cardToString()
lib.cardToString.argtypes = [ctypes.c_void_p]
lib.cardToString.restype = ctypes.c_char_p  # Returns a string

#lib.free.argtypes = [ctypes.c_void_p]
#lib.free.restype = None

lib.setCardFullName.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
lib.setCardFullName.restype = ctypes.c_int

lib.createEmptyCard.restype = ctypes.c_void_p  # Because it's returning a pointer (Card*)
lib.createEmptyCard.argtypes = []  # No parameters

lib.getCardBirthdayRaw.argtypes = [ctypes.c_void_p]
lib.getCardBirthdayRaw.restype = ctypes.c_void_p

lib.getCardAnniversaryRaw.argtypes = [ctypes.c_void_p]
lib.getCardAnniversaryRaw.restype = ctypes.c_void_p

#lib.getCardBirthday.argtypes = [ctypes.c_void_p]
#lib.getCardBirthday.restype = ctypes.c_char_p

#lib.getCardAnniversary.argtypes = [ctypes.c_void_p]
#lib.getCardAnniversary.restype = ctypes.c_char_p



def initialize_database(conn):
    cursor = conn.cursor()


    # Create FILE table
    create_file_table = '''
        CREATE TABLE IF NOT EXISTS FILE (
            file_id INT AUTO_INCREMENT PRIMARY KEY,
            file_name VARCHAR(60) NOT NULL,
            last_modified DATETIME,
            creation_time DATETIME NOT NULL
        );
    '''

    # Create CONTACT table
    create_contact_table = '''
        CREATE TABLE IF NOT EXISTS CONTACT (
            contact_id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(256) NOT NULL,
            birthday DATETIME,
            anniversary DATETIME,
            file_id INT NOT NULL,
            FOREIGN KEY (file_id) REFERENCES FILE(file_id) ON DELETE CASCADE
        );
    '''

    try:
        cursor.execute(create_file_table)
        cursor.execute(create_contact_table)
        conn.commit()
    except mysql.connector.Error as err:
        print(f"Error creating tables: {err}")
    finally:
        cursor.close()


# Global variable to store the selected filename
selected_filename = None

# Function to get valid vCard files from the 'cards/' directory
def get_valid_vcards():
    valid_files = []
    directory = "./cards/"  # Ensure cards/ folder exists in bin/
    
    if not os.path.exists(directory):
        return valid_files  # Return empty if folder doesn't exist

    for file in os.listdir(directory):
        if file.endswith(".vcf"):  # Only consider vCard files
            file_path = os.path.join(directory, file).encode('utf-8')  # Convert to C-style string
            card_ptr = ctypes.c_void_p()  # Pointer for storing Card object
            
            # Call createCard function from C
            create_result = lib.createCard(file_path, ctypes.byref(card_ptr))
            
            if create_result != 0:  # 0 means OK
                continue
            
            # Call validateCard function
            validate_result = lib.validateCard(card_ptr)
            if validate_result != 0:  # 0 means valid
                continue

                # --- DB Insertion logic below ---
            if 'db_connection' in globals() and db_connection:
                try:
                    cursor = db_connection.cursor()

                    # Check if file already exists
                    check_query = "SELECT file_id FROM FILE WHERE file_name = %s"
                    cursor.execute(check_query, (file,))
                    result = cursor.fetchone()

                    if result:
                        valid_files.append((file, file))
                        continue # the file is already processed so skip everything below aka all the inserting logic
                
                    # Gather file timestamps
                    last_modified = datetime.datetime.fromtimestamp(os.path.getmtime(os.path.join(directory, file)))
                    creation_time = datetime.datetime.now()

                    #Insert the file into FILE table
                    insert_file = '''
                        INSERT INTO FILE (file_name, last_modified, creation_time)
                        VALUES (%s, %s, %s)
                    '''
                    cursor.execute(insert_file, (file, last_modified, creation_time))
                    db_connection.commit()
                    file_id = cursor.lastrowid  # get inserted file_id

                    # Extract full name
                    fn_ptr = lib.getCardFullName(card_ptr)
                    full_name = ctypes.string_at(fn_ptr).decode('utf-8') if fn_ptr else "Unknown"
                    if fn_ptr:
                        lib.free(fn_ptr)

                    # Extract birthday
                    birthday = None
                    bday_ptr = lib.getCardBirthdayRaw(card_ptr)
                    if bday_ptr:
                        bday_str = ctypes.string_at(bday_ptr).decode('utf-8')
                        if bday_str.strip() != "":
                            try:
                                birthday = datetime.datetime.strptime(bday_str, "%Y%m%dT%H%M%S")
                            except ValueError:
                                birthday = None
                        lib.free(bday_ptr)

                    # Extract anniversary
                    anniversary = None
                    ann_ptr = lib.getCardAnniversaryRaw(card_ptr)
                    if ann_ptr:
                        ann_str = ctypes.string_at(ann_ptr).decode('utf-8')
                        if ann_str.strip() != "":
                            try:
                                anniversary = datetime.datetime.strptime(ann_str, "%Y%m%dT%H%M%S")
                            except ValueError:
                                anniversary = None
                        lib.free(ann_ptr)

                    # Insert into CONTACT table
                    insert_contact = '''
                        INSERT INTO CONTACT (name, birthday, anniversary, file_id)
                        VALUES (%s, %s, %s, %s)
                    '''
                    cursor.execute(insert_contact, (full_name, birthday, anniversary, file_id))
                    db_connection.commit()

                except mysql.connector.Error as err:
                    print(f"DB Error: {err}")
                finally:
                    cursor.close()

            # Only add to list if it passed all checks
            valid_files.append((file, file))

    return valid_files

# The Main UI Class (vCard List View)
class VCardListView(Frame):
    def __init__(self, screen):
        super(VCardListView, self).__init__(screen,
                                            screen.height * 2 // 3,
                                            screen.width * 2 // 3,
                                            on_load=self._reload_list,
                                            hover_focus=True,
                                            can_scroll=False,
                                            title="vCard List")

        # Store valid vCard files
        #self._vcard_files = get_valid_vcards()
        self._vcard_files = []

        # Create the list view for vCards
        self._list_view = ListBox(
            Widget.FILL_FRAME,
            self._vcard_files,
            name="vcard_list",
            add_scroll_bar=True,
            on_select=self._edit_card)

        # Create buttons layout
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        layout.add_widget(self._list_view)
        layout.add_widget(Divider())

        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Edit", self._edit_card), 0)
        layout2.add_widget(Button("Create", self._create_card), 1)
        layout2.add_widget(Button("DB Queries", self._db_query), 2)
        layout2.add_widget(Button("Exit", self._exit), 3)

        self.fix()

    def _reload_list(self, new_value=None):
        self._vcard_files = get_valid_vcards()
        self._list_view.options = self._vcard_files
        self._list_view.value = new_value

    def _edit_card(self):
        if self._list_view.value:
            self.save()
            filename = self._list_view.value # store the selected filename globally
            
            #raise NextScene("EditVCard",filename) # move on to edit the vcard... 
            #self.screen.set_scenes([Scene([VCardEditView(self.screen, filename)], -1, name="EditVCard")])
            self.screen.set_scenes([
              Scene([VCardListView(self.screen)], -1, name="Main"),
              Scene([VCardEditView(self.screen, filename)], -1, name="EditVCard")
            ])
            raise NextScene("EditVCard") # This actually switches the scenes

    def _create_card(self):
        raise NextScene("CreateVCard")
    
    def _db_query(self):
        self.screen.set_scenes([
            Scene([LoginView(self.screen)], -1, name="Login"),
            Scene([VCardListView(self.screen)], -1, name="Main"),
            Scene([VCardEditView(self.screen, "Unknown.vcf")], -1, name="EditVCard"),
            Scene([DBQueryView(self.screen)], -1, name="DBQuery"),
            Scene([CreateCardView(self.screen)], -1, name="CreateVCard")
        ])
        raise NextScene("DBQuery")
      
    @staticmethod
    def _exit():
        raise StopApplication("User exited")
    
    

class VCardEditView(Frame):
    def __init__(self, screen,filename): #accepts the filename as an argument
        super(VCardEditView, self).__init__(screen,
                                            screen.height * 2 // 3,
                                            screen.width * 2 // 3,
                                            hover_focus=True,
                                            can_scroll=False,
                                            title="vCard details")

        if not filename:
          filename = "Unknown.vcf"

        #self._model = model
        self._filename = filename # Store filename correctly
        self._card_ptr = ctypes.c_void_p()

        # Load the vCard file
        file_path = os.path.join("./cards/", filename).encode('utf-8')
        create_result = lib.createCard(file_path, ctypes.byref(self._card_ptr))
        # The default values which we need
        contact_name = "Unknown"
        birthday = ""
        anniversary = ""
        other_props_count = 0

        if create_result == 0:  # OK
          # Get Contact Name
          fn_ptr = lib.getCardFullName(self._card_ptr)
          if fn_ptr:
            contact_name = ctypes.string_at(fn_ptr).decode('utf-8')
            lib.free(fn_ptr) # free the allocated memory in C
          
          # Get Birthday
            bday_ptr = lib.getCardBirthday(self._card_ptr)
            if bday_ptr:
                birthday = ctypes.string_at(bday_ptr).decode('utf-8')
                lib.free(bday_ptr)  # Free allocated memory in C

            # Get Anniversary
            ann_ptr = lib.getCardAnniversary(self._card_ptr)
            if ann_ptr:
                anniversary = ctypes.string_at(ann_ptr).decode('utf-8')
                lib.free(ann_ptr)  # Free allocated memory in C

            # Get Other Properties Count
            other_props_count = lib.getOtherPropertiesCount(self._card_ptr)

        # UI Layout
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)

        # Labels for displaying details
        layout.add_widget(Label(f"File name:      {filename}"))
        self.fn_field = Text ("Contact:", "fn")
        self.fn_field.value = contact_name # set default name
        layout.add_widget(self.fn_field)
        #layout.add_widget(Label(f"Contact:        {contact_name}"))
        layout.add_widget(Label(f"Birthday:       {birthday}"))
        layout.add_widget(Label(f"Anniversary:    {anniversary}"))
        layout.add_widget(Label(f"Other properties: {other_props_count}"))
        layout.add_widget(Divider())
     

        # Buttons 
        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._save_card), 0) # Calls _save_card
        layout2.add_widget(Button("Cancel", self._cancel), 1) # Calls _cancel 

        self.fix()
        #self.reset()
    
    def _exit(self):
      self.screen.set_scenes([
        Scene([VCardListView(self.screen)], -1, name="Main"),
        Scene([VCardEditView(self.screen, self._filename)], -1, name="EditVCard")
      ])
      raise NextScene("Main")


    def _on_change(self):
        pass  # can handle real-time validation here if needed

    def _save_card(self):
        self.save() #Ensures that the data is saved from UI widgets
        new_name = self.fn_field.value.strip() #Gets the edited name

        if not new_name: #Prevents any empty names
          self.fn_field.value = "Invalid Name!"
          return
        
         # Call C function to update the FN property
        result = lib.setCardFullName(self._card_ptr, new_name.encode('utf-8'))  # Implemented this in C

        if result !=0: 
            self.fn_field.value = "Error saving!"
            return
        
        # Write card to disk
        file_path = os.path.join("./cards/", self._filename)
        lib.writeCard(file_path.encode('utf-8'), self._card_ptr)

        # Update database with new file and contact info
    
        if 'db_connection' in globals() and db_connection:
            try:
                cursor = db_connection.cursor()

                # Get new last_modified timestamp
                last_modified = datetime.datetime.fromtimestamp(os.path.getmtime(file_path))

                # Get updated name (again, just to be sure)
                fn_ptr = lib.getCardFullName(self._card_ptr)
                name = ctypes.string_at(fn_ptr).decode('utf-8') if fn_ptr else "Unknown"
                if fn_ptr:
                    lib.free(fn_ptr)

                # Get birthday
                birthday = None
                bday_ptr = lib.getCardBirthdayRaw(self._card_ptr)
                if bday_ptr:
                    bday_str = ctypes.string_at(bday_ptr).decode('utf-8')
                    try:
                        birthday = datetime.datetime.strptime(bday_str, "%Y%m%dT%H%M%S")
                    except ValueError:
                        birthday = None
                    lib.free(bday_ptr)

                # Get anniversary
                anniversary = None
                ann_ptr = lib.getCardAnniversaryRaw(self._card_ptr)
                if ann_ptr:
                    ann_str = ctypes.string_at(ann_ptr).decode('utf-8')
                    try:
                        anniversary = datetime.datetime.strptime(ann_str, "%Y%m%dT%H%M%S")
                    except ValueError:
                        anniversary = None
                    lib.free(ann_ptr)

                # Update FILE table with new last_modified
                cursor.execute(
                    "UPDATE FILE SET last_modified = %s WHERE file_name = %s",
                    (last_modified, self._filename)
                )

                # Update CONTACT table
                cursor.execute('''
                    UPDATE CONTACT SET name = %s, birthday = %s, anniversary = %s
                    WHERE file_id = (SELECT file_id FROM FILE WHERE file_name = %s)
                ''', (name, birthday, anniversary, self._filename))

                db_connection.commit()
                cursor.close()

            except mysql.connector.Error as err:
                print(f"DB Update Error: {err}")

        # Switch back to main scene
        raise NextScene("Main")

    # Switch back to main scene
    def _cancel(self):
        raise NextScene("Main")




class CreateCardView(Frame):
    def __init__(self, screen):
        super(CreateCardView, self).__init__(screen,
                                             screen.height * 2 // 3,
                                             screen.width * 2 // 3,
                                             hover_focus=True,
                                             can_scroll=False,
                                             title="Create New vCard")

        self._card_ptr = ctypes.c_void_p()

        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)

        self._filename_field = Text("File name (.vcf):", "filename")
        self._fn_field = Text("Contact name:", "fn")

        layout.add_widget(self._filename_field)
        layout.add_widget(self._fn_field)
        layout.add_widget(Divider())

        self._error_label = Label("", align="^")
        layout.add_widget(self._error_label)

        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._create_card), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)

        self.fix()

    def _create_card(self):
        self.save()
        filename = self.data.get("filename", "").strip()
        fn_name = self.data.get("fn", "").strip()

        self._error_label.text = "" # Clears any previous that caused as error

        if not filename.endswith(".vcf"):
            self._error_label.text = "Filename must end in .vcf"
            self.fix()
            return

        filepath = os.path.join("./cards", filename)

        if os.path.exists(filepath):
            self._error_label.text = "File already exists!"
            self.fix()
            return

        if not fn_name:
            self._error_label.text = "Name required"
            self.fix()
            return

        # Create the card using the C function that returns a pointer directly
        self._card_ptr = lib.createEmptyCard()
        if not self._card_ptr:
            self._error_label.text = "Failed to create card"
            self.fix()
            return

        # Set full name
        set_result = lib.setCardFullName(self._card_ptr, fn_name.encode("utf-8"))
        if set_result != 0:
            self._error_label.text = "Invalid name!"
            self.fix()
            return

        # Validate card
        valid = lib.validateCard(self._card_ptr)
        if valid != 0:
            self._error_label.text = "Card invalid!"
            self.fix()
            return

        # Write card to disk
        written = lib.writeCard(filepath.encode("utf-8"), self._card_ptr)
        if written != 0:
            self._error_label.text = "Failed to save card"
            self.fix()
            return

        raise NextScene("Main")  
    
    def reset(self):
      super(CreateCardView, self).reset()
      self._filename_field.value = ""
      self._fn_field.value = ""
      self._error_label.text = ""
      self.fix()


    @staticmethod
    def _cancel():
        raise NextScene("Main")



class DBQueryView(Frame):
    def __init__(self, screen):
        super(DBQueryView, self).__init__(screen,
                                          screen.height * 2 // 3, # or change to just // 2 if I want to reduce the height
                                          screen.width * 2 // 3,
                                          hover_focus=True,
                                          can_scroll=False,
                                          title="Database Queries")

        # Main layout for query results (DO NOT fill entire frame)**
        layout = Layout([1])
        self.add_layout(layout)

        # Text box for displaying query results
        self._query_results = TextBox(height=6, line_wrap=True, as_string=True, name="results")
       

        layout.add_widget(Label("Query Results:"))
        layout.add_widget(self._query_results)
        layout.add_widget(Label(""))  # Spacer
        # layout.add_widget(Divider())

        # Buttons for queries and navigation
        layout2 = Layout([1, 1, 1])
        self.add_layout(layout2)

        layout2.add_widget(Button("Display all contacts", self._display_all), 0)
        layout2.add_widget(Button("Find contacts born in June", self._find_june), 1)
        layout2.add_widget(Button("Cancel", self._go_back), 2)

        self.fix()

    def _display_all(self):
        if 'db_connection' not in globals() or db_connection is None:
            self._query_results.value = "No database connection, please log in first"
            return
        
        try:
            cursor = db_connection.cursor()
            query = '''
                SELECT c.name, c.birthday, c.anniversary, f.file_name
                FROM CONTACT c
                JOIN FILE f ON c.file_id = f.file_id
                ORDER BY c.name
            '''
            cursor.execute(query)
            rows = cursor.fetchall()
            #cursor.close()

            output = f"{'Name':<25} {'Birthday':<20} {'Anniversary':<20} {'File Name':<30}\n"
            output += "="*95 + "\n"

            for row in rows:
                name = str(row[0]) if row[0] else "NULL"
                birthday = str(row[1]) if row[1] else "NULL"
                anniversary = str(row[2]) if row[2] else "NULL"
                filename = str(row[3]) if row[3] else "NULL"

                output += f"{name:<25} {birthday:<20} {anniversary:<20} {filename:<30}\n"

            # Status summary
            cursor.execute("SELECT COUNT(*) FROM FILE")
            num_files = cursor.fetchone()[0]
            cursor.execute("SELECT COUNT(*) FROM CONTACT")
            num_contacts = cursor.fetchone()[0]
            cursor.close()

            output += f"\nDatabase has {num_files} files and {num_contacts} contacts."


            self._query_results.value = output
            
        
        except mysql.connector.Error as err:
            self._query_results.value = f"Query failed: {err}" 
            

    def _find_june(self):
        if 'db_connection' not in globals() or db_connection is None:
            self._query_results.value = "No database connection, please log in first"
            return
        
        try:
            cursor = db_connection.cursor()
            query = '''
                SELECT c.name, c.birthday, TIMESTAMPDIFF(YEAR, c.birthday, f.last_modified) AS age
                FROM CONTACT c
                JOIN FILE f ON c.file_id = f.file_id
                WHERE MONTH(c.birthday) = 6
                ORDER BY age DESC
            '''
            cursor.execute(query)
            rows = cursor.fetchall()
            #cursor.close()


            output = f"{'Name':<25} {'Birthday':<20}\n"
            output += "="*45 + "\n"
            for row in rows:
                name = str(row[0]) if row[0] else "NULL"
                birthday = str(row[1]) if row[1] else "NULL"
                output += f"{name:<20} {birthday}\n"   

            # Status summary
            cursor.execute("SELECT COUNT(*) FROM FILE")
            num_files = cursor.fetchone()[0]
            cursor.execute("SELECT COUNT(*) FROM CONTACT")
            num_contacts = cursor.fetchone()[0]
            cursor.close()

            output += f"\nDatabase has {num_files} files and {num_contacts} contacts."   

            self._query_results.value = output
           
        
        except mysql.connector.Error as err:
            self._query_results.value = f"Query failed: {err}" 
            


    def _go_back(self):
        raise NextScene("Main")



class LoginView(Frame):
    def __init__(self, screen):
        super(LoginView, self).__init__(screen,
                                        screen.height * 2 // 3,
                                        screen.width * 2 // 3,
                                        hover_focus=True,
                                        can_scroll=False,
                                        title="Login to Database")

        self._connection = None  # Will store the DB connection

        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)

        self._user_field = Text("Username:", name="user")
        self._pass_field = Text("Password:", "password", hide_char="*")
        self._db_field = Text("Database:", name="db")

        layout.add_widget(self._user_field)
        layout.add_widget(self._pass_field)
        layout.add_widget(self._db_field)
        layout.add_widget(Divider())


        # Add a label for showing errors
        self._error_label = Label("", align="^")
        layout.add_widget(self._error_label)

        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._connect), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)

        self.fix()

    def _connect(self):
        self.save()
        user = self.data["user"].strip()
        passwd = self.data["password"].strip()
        db = self.data["db"].strip()

        #Clear any old error message
        self._error_label.text = ""

        try:
            conn = mysql.connector.connect(
                host="dursley.socs.uoguelph.ca",
                user=user,
                password=passwd,
                database=db
            )
            conn.autocommit = True
            initialize_database(conn)
            global db_connection  # Store for use in other modules
            db_connection = conn
            raise NextScene("Main")
        except mysql.connector.Error as err:
            self._user_field.value = ""   #Clearing fields so they can try again if values are incorrect
            self._pass_field.value = ""
            self._db_field.value = "" 
            self._error_label.text = "Login failed. Invalid credentials or database name. Try again:"
            self.fix() #Refresh frame to show new values

    @staticmethod
    def _cancel():
        global db_connection
        db_connection = None #Marks that the DB is not connected
        raise NextScene("Main")



# Function to run the UI
def demo(screen, scene, filename=None):
    scenes = [
        Scene([LoginView(screen)], -1, name="Login"),
        Scene([VCardListView(screen)], -1, name="Main"),
        Scene([VCardEditView(screen,filename if filename else "Unknown.vcf")],-1, name="EditVCard"), 
        Scene([DBQueryView(screen)], -1, name="DBQuery"),
        Scene([CreateCardView(screen)], -1, name="CreateVCard")
    ]

    # Find the actual Scene object that matches the name
    scene_name = scene or "Login"
    start_scene = next((s for s in scenes if s.name == scene_name), scenes[0]) # iterates through the scenes array list to find login other sets start scene to login by default
    screen.play(scenes, stop_on_resize=True, start_scene=start_scene, allow_int=True)


# Run the UI
if __name__ == "__main__":
    Screen.wrapper(demo,arguments=[None])

