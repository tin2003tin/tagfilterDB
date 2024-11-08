CREATE TYPE RoleType AS ENUM (
    'Viewer',   -- User who views the properties
    'Owner',    -- User who owns properties
    'ADMIN'     -- User with administrative privileges
);

CREATE TYPE HouseType AS ENUM (
    'Apartment',    -- Apartment type house
    'Villa',        -- Villa type house
    'Townhouse',    -- Townhouse type house
    'Detached',     -- Detached house
    'SemiDetached', -- Semi-detached house
    'Bungalow',     -- Bungalow type house
    'Cottage',      -- Cottage type house
    'Mansion'       -- Mansion type house
);

CREATE TYPE RoomType AS ENUM (
    'Bedroom',    -- A room used for sleeping
    'Kitchen',    -- A room for cooking
    'LivingRoom', -- A room for relaxing or socializing
    'Bathroom',   -- A room for personal hygiene
    'DiningRoom', -- A room for dining
    'Office',     -- A room for working or studying
    'Hallway',    -- A passageway or corridor in a house
    'Storage',    -- A room for storing items
    'Laundry',    -- A room for washing clothes
    'Garage'      -- A room for parking vehicles
);

CREATE TABLE Account (
    AccountID INT PRIMARY KEY, -- Unique identifier for each account.
    Username VARCHAR(50) UNIQUE NOT NULL, -- A unique username used for login.
    Email VARCHAR(100) UNIQUE NOT NULL, -- The user's email address, also unique.
    PasswordHash VARCHAR(255) NOT NULL, -- A hashed version of the user’s password.
    Salt VARCHAR(50) NOT NULL, -- A random string combined with the password to ensure hashed.
    Role RoleType NOT NULL,-- Defines the type of account.
    CreatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP -- A timestamp recording when the account was created.
);

CREATE TABLE Viewer (
    ViewerID INT PRIMARY KEY, -- Unique identifier for each viewer
    Preferences JSON, -- A JSON field storing user-specific preferences or filters for property searches, like preferred locations or price range.
    FOREIGN KEY (ViewerID) REFERENCES Account(AccountID) ON DELETE CASCADE
);

CREATE TABLE Owner (
    OwnerID INT PRIMARY KEY,
    ContactNumber VARCHAR(20), -- The contact number of the owner.
    Address VARCHAR(255), -- The physical address of the owner
    PropertiesOwned JSON, --  A JSON field containing an array of property identifiers (e.g., HouseIDs) indicating which properties the owner owns. This allows flexibility to store multiple owned properties in one field.
    FOREIGN KEY (OwnerID) REFERENCES Account(AccountID) ON DELETE CASCADE
);

CREATE TABLE House (
    HouseID INT PRIMARY KEY, 
	HouseName VARCHAR(50) UNIQUE NOT NULL,
    Location VARCHAR(255) NOT NULL, --  The address or geographical location of the house.
    OwnerID INT, -- A foreign key linking to the Owner table
	Height DECIMAL(10, 2) NOT NULL,
	Weight DECIMAL(10, 2) NOT NULL,
    NumberOfRooms INT, --  The total number of rooms in the house
    ConstructionYear INT, -- The year the house was built
    Price DECIMAL(15, 2), -- The market value of the house
    HouseType HouseType, --  Describes the type of house (e.g., "Apartment," "Villa," "Townhouse"). This can be useful for categorization or filtering properties by type.
    Neighborhood VARCHAR(100), -- The name of the neighborhood or community where the house is located.
    UtilitiesAvailable JSON, -- A JSON object that stores available utilities (e.g., electricity, water, internet, etc.). The use of JSON allows for flexibility in representing a variable number of utilities without requiring a fixed set of columns.
    FOREIGN KEY (OwnerID) REFERENCES Owner(OwnerID) 
);

CREATE TABLE HouseDetails (
    HouseID INT PRIMARY KEY, -- Foreign key to the House table
    Description TEXT, -- General description or detailed information about the house
    YearRenovated INT, -- Year of the most recent renovation, if applicable
    FloorCount INT, -- Number of floors in the house
    HasGarage BOOLEAN, -- Indicates if there is a garage
    HasGarden BOOLEAN, -- Indicates if there is a garden
    AdditionalFeatures JSON, -- JSON to store other features (e.g., "fireplace", "swimming pool")
    FOREIGN KEY (HouseID) REFERENCES House(HouseID) ON DELETE CASCADE
);

CREATE TABLE Room (
	RoomId INT PRIMARY KEY,
    RoomName VARCHAR(50),      -- Room name within a house
    HouseID INT,               -- Foreign key to the House table
    RoomType RoomType,         -- Type of room (e.g., Bedroom, Kitchen)
    FloorNumber INT,           -- Which floor the room is on
    Area DECIMAL(10, 2),       -- Area of the room in square footage
    Dimensions JSON,           -- JSON for storing spatial boundaries
    HasWindow BOOLEAN,         -- Indicates if the room has a window
    FOREIGN KEY (HouseID) REFERENCES House(HouseID) ON DELETE CASCADE  -- Foreign key to the House table
);

CREATE TABLE Image (
    ImageID SERIAL PRIMARY KEY,
    HouseID INT, -- Foreign key to the House table
    RoomId INT, -- Foreign key to the Room table, if applicable
    ImagePath VARCHAR(255), -- Path to the image file or URL if stored externally
    Description TEXT, -- Brief description of the image (e.g., "Front View of House", "Bedroom Interior")
    DateTaken DATE, -- Date when the image was taken
    FOREIGN KEY (HouseID) REFERENCES House(HouseID) ON DELETE CASCADE,
    FOREIGN KEY (RoomId) REFERENCES Room(RoomId) ON DELETE CASCADE
);

-- Insert example accounts
INSERT INTO Account (AccountID, Username, Email, PasswordHash, Salt, Role) VALUES
(1,'john_doe', 'john.doe@example.com', 'hashed_password_1', 'salt_1', 'Viewer'),
(2,'jane_smith', 'jane.smith@example.com', 'hashed_password_2', 'salt_2', 'Owner'),
(3,'admin_user', 'admin@example.com', 'hashed_password_3', 'salt_3', 'ADMIN');

-- Insert example viewer data
INSERT INTO Viewer (ViewerID, Preferences) VALUES
(1, '{"preferred_location": "Downtown", "price_range": {"min": 500000, "max": 1000000}}');

-- Insert example owner data
INSERT INTO Owner (OwnerID, ContactNumber, Address, PropertiesOwned) VALUES
(2, '123-456-7890', '123 Main St, Springfield, IL', '{"HouseIDs": [101, 102]}');


-- Example 1
-- Inserting into the House table
INSERT INTO House
VALUES 
(1,'Sunny Villa', '123 Maple Street, Springfield', 2, 250.00, 250.00, 6, 2015, 500000.00, 'Villa', 'Greenwood', '{"electricity": true, "water": true, "internet": true}');

-- Inserting into the HouseDetails table
INSERT INTO HouseDetails
VALUES 
(1, 'A beautiful 6-room villa with modern design and spacious backyard.', 2019, 2, true, true, '{"swimming pool": true, "fireplace": true}');

-- Inserting into the Room table
INSERT INTO Room
VALUES 
(1, 'Living Room', 1, 'LivingRoom', 1, 35.00, '{"x_min": 0, "y_min": 0, "x_max": 7, "y_max": 5}', true),
(2, 'Bedroom', 1, 'Bedroom', 2, 20.00, '{"x_min": 0, "y_min": 0, "x_max": 5, "y_max": 4}', true),
(3, 'Kitchen', 1, 'Kitchen', 1, 15.00, '{"x_min": 0, "y_min": 0, "x_max": 4, "y_max": 3}', false);

-- Inserting into the Image table
INSERT INTO Image (HouseID, RoomId, ImagePath, Description ,DateTaken)
VALUES 
(1, 1, '/images/sunny_villa_front.jpg', 'Front view of Sunny Villa', '2024-11-06'),
(1, 2, '/images/living_room1.jpg', 'Spacious living room with modern furniture', '2024-11-06'),
(1, 3, '/images/bedroom1.jpg', 'Master bedroom with cozy furnishings', '2024-11-06');


-- Example 2
-- Inserting into the House table
INSERT INTO House
VALUES 
(2, 'Cozy Townhouse', '456 Oak Avenue, Los Angeles', 2, 180.00, 180.00, 5, 2010, 350000.00, 'Townhouse', 'Sunset Hills', '{"electricity": true, "water": true, "gas": true}');

-- Inserting into the HouseDetails table
INSERT INTO HouseDetails
VALUES 
(2, 'A modern townhouse with ample living space and a nice garden.', 2018, 3, true, true, '{"balcony": true, "home theater": true}');

-- Inserting into the Room table
INSERT INTO Room
VALUES 
(4, 'Living Room', 2, 'LivingRoom', 1, 40.00, '{"x_min": 0, "y_min": 0, "x_max": 8, "y_max": 5}', true),
(5, 'Bathroom', 2, 'Bathroom', 2, 10.00, '{"x_min": 0, "y_min": 0, "x_max": 3, "y_max": 3}', true),
(6, 'Dining Room', 2, 'DiningRoom', 1, 20.00, '{"x_min": 0, "y_min": 0, "x_max": 5, "y_max": 4}', true);

-- Inserting into the Image table
INSERT INTO Image (HouseID, RoomId, ImagePath, Description ,DateTaken)
VALUES 
(2, 4, '/images/cozy_townhouse_front.jpg', 'Front view of Cozy Townhouse', '2024-11-06'),
(2, 5, '/images/living_room2.jpg', 'Spacious living room with elegant design', '2024-11-06'),
(2, 6, '/images/dining_room2.jpg', 'Dining room with ample seating and bright lighting', '2024-11-06');


-- Example 3
-- Inserting into the House table
INSERT INTO House
VALUES 
(3, 'Modern Apartment', '789 Pine Road, New York', 2, 120.00, 120.00, 4, 2022, 600000.00, 'Apartment', 'Manhattan', '{"electricity": true, "water": true, "internet": true, "gas": true}');

-- Inserting into the HouseDetails table
INSERT INTO HouseDetails 
VALUES 
(3, 'A chic apartment with stunning city views and high-end finishes.', NULL, 1, false, false, '{"elevator": true, "gym": true}');

-- Inserting into the Room table
INSERT INTO Room 
VALUES 
(7, 'Living Room', 3, 'LivingRoom', 1, 25.00, '{"x_min": 0, "y_min": 0, "x_max": 5, "y_max": 5}', true),
(8, 'Bedroom', 3, 'Bedroom', 1, 15.00, '{"x_min": 0, "y_min": 0, "x_max": 4, "y_max": 3}', true),
(9, 'Bathroom', 3, 'Bathroom', 1, 8.00, '{"x_min": 0, "y_min": 0, "x_max": 3, "y_max": 2}', true);

-- Inserting into the Image table
INSERT INTO Image (HouseID, RoomId, ImagePath, Description ,DateTaken)
VALUES 
(3, 7, '/images/modern_apartment_view.jpg', 'City view from the Modern Apartment', '2024-11-06'),
(3, 8, '/images/living_room3.jpg', 'Living room with large windows and city skyline', '2024-11-06'),
(3, 9, '/images/bedroom3.jpg', 'Comfortable bedroom with soft lighting', '2024-11-06');