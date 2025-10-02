# Myshop XML Tools
## How to build:​
1. Have **Visual studio 2022 installed**;​
2. Make sure you have **Desktop C++** and **ALL the VS22 (143**) components installed;​
3. Clone this repository, and extract the downloaded .zip in a new folder;​
4. Open the **Trickster SQL to XML** or the **Trickster XML to SQL** solution;
5. **Build** the solution, the .exe will be inside the **Output folder**;

## How to use:
1. Download the latest release (or build yourself from source);
2. If you want to generate SQL from your Myshop XML, put the **libcmgds_e.xml** in the same folder as the **XML to SQL.exe** and run it, the .sql files will be in the **Output** folder;
3. If you want to generate a XML file from your SQL database, edit the DSN file, in **config/database.dsn** with your SQL server config, then run **SQL to XML.exe**, it will generate a new **libcmgds_e.xml**;
