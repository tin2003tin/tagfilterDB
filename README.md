# TagFilterDB Project

<img src="https://github.com/user-attachments/assets/c88d24be-3342-44a1-851d-6aea4f4e0459" width="500" />

## Overview

TagFilterDB is a advanced database system. It designed for efficient tagging and filtering of multidimensional data, uses dynamic data management and powerful querying capabilities to handle complicated datasets. This database is designed to optimize both spatial and non-spatial searches, and it supports a broad range of data types and structures, TagFilterDB provides complete data management and impressive performance for a variety of analytical applications by including R*-tree structure visualization tools and interface with file management components.


## Components

### 1: R*-Tree

**Data Structure:**  
An R-tree is employed to efficiently index and manage multidimensional data.

**Node Support:** 
Supports many node types for diverse data architectures. Future upgrades will provide more node kinds.

**Visualization:** 
Tools are provided to visualize the R-tree structure for easier troubleshooting and comprehension.

**Traversal:**  
Provides techniques for tree traversal, such as depth-first and breadth-first, for search and data retrieval.

### 2: DataManager

**Dynamic Collection:**  
Handles diverse data types and schemas, including custom-defined schemas.

**Tag Filtering:**  
Facilitates advanced tagging and filtering based on custom tags for optimized data retrieval and analysis.

**Encode/Decode:**  
Efficiently encodes and decodes data for storage and retrieval.

**Visualization:**  
Provides tools to visualize data management operations for improved monitoring and analysis.

**Custom Schema Design:**  
Enables the creation of data schemas using a specialized language and compiler for tailored data handling.

### 3. QueryEngine

**Query Processing:**  
Efficiently processes searches for various data types using optimized techniques.

**Tag Filtering:**  
Supports advanced tagging and filtering based on custom criteria and tags, integrating seamlessly with the R-tree and DataManager.

**Optimization:**  
Employs techniques like indexing and caching to enhance query performance.

**Result Management:**  
Manages and formats query results with support for pagination, sorting, and grouping.

### 4. FileManager (Integrated with BufferManager, LogManager, Monitoring)

**File Management:**  
Handles file operations, including creation, reading, writing, and deletion, ensuring performance and data integrity.

**BufferManager Integration:**  
Manages in-memory buffers to reduce direct file I/O and maintain data consistency.

**LogManager Integration:**  
Maintains detailed logs of file operations, supporting recovery and debugging with write-ahead logging (WAL).

**Monitoring Integration:**  
Provides real-time monitoring of file I/O, buffer usage, and log status, with alerts and reports on system health.

**File Security:**  
Implements encryption and access control to protect sensitive data and ensure authorized access.


## Contact

For questions or support, please contact [tinsiwid@gmail.com].

## Reference
r-star-tree - https://github.com/virtuald/r-star-tree/blob/master/RStarTree.h#L290
