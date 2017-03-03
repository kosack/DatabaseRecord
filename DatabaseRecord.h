//
// DatabaseRecord class using sqlite3
// Karl Kosack, 2005
//

#ifndef DATABASERECORD_H
#define DATABASERECORD_H

#include <map>
#include <string>
#include <vector>
#include <sqlite3.h>
#include <stdexcept>

enum DatabaseFieldType {FIELD_INT, FIELD_DOUBLE, FIELD_STRING};

struct DatabaseField {
    void *ptr;
    DatabaseFieldType type;
    bool primary_key;
};


typedef sqlite3* database_t ;

/**
 * Wrapper class for the database; eventually, this should encapsulate
 * all calls to sqlite3, so the other stuff is independent, and the
 * database engine can be changed.
 */
class Database {

 public:
    Database( std::string filename ) {
	if (sqlite3_open( filename.c_str(), &_db )) {
	    throw std::runtime_error("Couldn't open database '"+filename
				+"' because: "+sqlite3_errmsg(_db));
	}
   }

    ~Database() {
	if (sqlite3_close(_db))
	    std::cout << "CLOSE: "<<sqlite3_errmsg(_db)<<std::endl;
	
    }

    database_t getHandle() {return _db;}
    
 private:
    database_t _db;


};


/**
 * DatabaseRecord is a base class for generating "smart" structs which
 * can be written and read automatically from an sqlite3 database. To
 * use it, you should create your own subclass containing any public
 * variables you want.  In the constructor, you should call
 * DatabaseRecord::addField() for each variable, and specify the
 * type and a pointer to the data. You should also call
 * DatabaseRecord::setTableName() to set the name of the table to
 * read/write from in the database.  Then, you can call the
 * DatabaseRecord methods from your new class to write and read the
 * data.
 *
 * Before doing anything with your subclass of DatabaseRecord, you
 * must call the setDatabaseHandle() function and pass it a pointer to
 * an open database, otherwise the read and write functions will fail.
 *
 * example: 
 */
class DatabaseRecord {

 public:
    
    DatabaseRecord(): _write_in_progress(false),_read_in_progress(false),
	_writecount(0), _db(NULL),_tablename("unnamed_table"),
	_badcount(0){;}
    ~DatabaseRecord(){ finish();}

    void prepareToRead( std::string where_clause="" );
    int  readFromDatabase();
    void writeToDatabase();
    void setDatabaseHandle( database_t db ){
	_db=db; 
	if(!tableExists()) createTable();
    }
    int  getNumFields() { return _fieldmap.size();}
    void clearTable();
    void finish();
    int  count(std::string where="");
    std::ostream& print(std::ostream&);
    void zero();

    friend std::ostream& operator<<( std::ostream &stream,DatabaseRecord &rec );

 protected:
    void setTableName(std::string name){_tablename=name;}

    void addField( std::string name, int &variable ) {
	DatabaseField f;
	f.ptr = (void*) &variable;
	f.type = FIELD_INT;
	f.primary_key = false;
	_fieldmap[name] = f;
    }

    void addField( std::string name, double &variable ) {
	DatabaseField f;
	f.ptr = (void*) &variable;
	f.type = FIELD_DOUBLE;
	f.primary_key = false;
	_fieldmap[name] = f;
    }
    void addField( std::string name, std::string &variable ) {
	DatabaseField f;
	f.ptr = (void*) &variable;
	f.type = FIELD_STRING;
	f.primary_key = false;
	_fieldmap[name] = f;
    }

 private:

    void createTable();
    bool tableExists();
    std::string getSchema();
    std::string getFieldList();
    void prepareToWrite();
    
    database_t _db;
    sqlite3_stmt *_rdstmt, *_wrstmt;
    std::string _tablename;
    std::map< std::string, DatabaseField > _fieldmap;

    bool _write_in_progress;
    bool _read_in_progress;
    int _writecount;
    int _badcount;

};


std::string join( std::string delim, std::vector< std::string > &strvect );

#endif
