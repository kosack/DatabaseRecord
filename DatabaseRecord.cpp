//
// DatabaseRecord class using sqlite3
// Karl Kosack, 2005
//


#include <iostream>
#include <sqlite3.h>
#include <string>
#include <iomanip>
#include <map>
#include <sstream>

#include  "DatabaseRecord.h"
using namespace std;

/**
 * Call this to write the currently mapped values of your structure to
 * the database.
 */
void DatabaseRecord::writeToDatabase() {

    std::map< std::string, DatabaseField >::iterator it;
    int i=1;
    int ret;

    if (_db == NULL) throw runtime_error("NO DATABASE CONNECTION!");

    if (_write_in_progress == false) {
	prepareToWrite();
    }

    for (it=_fieldmap.begin(); it != _fieldmap.end(); it++) {
	switch (it->second.type) {
	case FIELD_INT:
	    sqlite3_bind_int(_wrstmt, i, *((int*)it->second.ptr) );
	    break;
	case FIELD_DOUBLE:
	    sqlite3_bind_double(_wrstmt, i, *((double*)it->second.ptr) );
	    break;
	case FIELD_STRING:
	    sqlite3_bind_text(_wrstmt, i, 
			      ((std::string*)it->second.ptr)->c_str(), 
			      ((std::string*)it->second.ptr)->length(), 
			      NULL );
	    break;
	}
	i++;
    }

    ret = sqlite3_step(_wrstmt) ;

    // handle errors.  This is somewhat messy, but seems to work ok.
    // For some reason, you sometimes get a error when writing two
    // different tables in succession.  Re-trying seems to fix it.  I
    // bail out after an arbitrary number of tries, just in case it
    // becomes an infinite loop

    if (ret != SQLITE_DONE) {
	if (ret == SQLITE_ERROR || ret == SQLITE_SCHEMA) {
	    cout << "DEBUG: write error: '"<<sqlite3_errmsg(_db)
		 <<"', attempting to re-issue write.."	 <<endl;
	    _badcount++; 
	    if (_badcount > 1000) {
		throw runtime_error("writeToDatabase() bailing out due "
				    "to too many bad writes on '"
				    +_tablename+"': "+sqlite3_errmsg(_db));
	    }
	    finish();
	    writeToDatabase();
	    return;
	} 
	else {
	    cout << "DEBUG: writeToDatabase() ret="<<ret<<endl;
	    throw runtime_error("writeToDatabase() on '"+_tablename+"': "
			    +sqlite3_errmsg(_db));
	}
    }
    sqlite3_reset(_wrstmt);
    _writecount++;

}


/**
 * Returns an SQL schema string for the table
 */
string DatabaseRecord::getSchema() {

    vector<string> fields;
    string tmp;
    std::map< std::string, DatabaseField >::iterator it;

    for (it=_fieldmap.begin(); it != _fieldmap.end(); it++) {
	if (it->second.primary_key==true) 
	    tmp ="PRIMARY KEY";
	else tmp = "";

	switch (it->second.type) {
	case FIELD_INT:
	    fields.push_back( it->first + " INTEGER " + tmp );
	    break;
	case FIELD_DOUBLE:
	    fields.push_back( it->first + " DOUBLE " + tmp );
	    break;
	case FIELD_STRING:
	    fields.push_back( it->first + " TEXT " +tmp );
	    break;
	}

    }

    return join( ", ",fields ); 

}

/**
 * \returns a list of all mapped fields, separated by commas
 */
string DatabaseRecord::getFieldList() {
    vector<string> fields;
    std::map< std::string, DatabaseField >::iterator it;
    for (it=_fieldmap.begin(); it != _fieldmap.end(); it++) {
	fields.push_back(it->first);
    }

    return join(", ",fields );
}



/**
 * prepare to read from the database. This should be called before
 * calling readFromDatabase(). You can specify an optional "WHERE"
 * clause to limit the results.
 */
void 
DatabaseRecord::prepareToRead( std::string where_clause ) {

    string sql;
    int ret;

    if (_db == NULL) throw runtime_error("NO DATABASE CONNECTION!");

    sql = "SELECT "+getFieldList()+" FROM "+_tablename;
    if (where_clause != "") {
	sql.append(" WHERE "+where_clause );
    }

    ret = sqlite3_prepare( _db, sql.c_str(), sql.length(), &_rdstmt, NULL );
    if (ret!= SQLITE_OK) {
	throw runtime_error("prepareToRead(): couldn't prepare '"+sql+
			    "': "+sqlite3_errmsg(_db) );
    }

    _read_in_progress=true;

}


/**
 * Automatically called the first time writeToDatabase() is called
 */
void 
DatabaseRecord::prepareToWrite() {
    
    if (_db == NULL) throw runtime_error("NO DATABASE CONNECTION!");

    if (tableExists() == false) {
	cout << "DatabaseRecord: Table '"
	     <<_tablename<<"' doesn't exist, creating it..."<<endl;
	createTable();
    }

    string sql = "INSERT INTO "+_tablename+" ("+getFieldList()+") VALUES (";

    vector<string> tmp;
    for (int i=0; i<getNumFields(); i++) {
	tmp.push_back("?");
    }

    sql.append( join(",",tmp) );
    sql.append(")");

    if(sqlite3_prepare( _db, sql.c_str(), sql.length(), &_wrstmt, NULL ) 
       != SQLITE_OK) {
	throw runtime_error("prepareToWrite(): sql error with '"+sql+"': "
			    +sqlite3_errmsg(_db));
    }

    sqlite3_exec( _db, "BEGIN TRANSACTION", NULL, NULL, NULL );

    _write_in_progress= true;

}


/**
 * End all database transactions.  This is called automatically when a
 * DatabaseRecord is deleted, but can be called manually if needed to
 * finish up the output.
 */
void
DatabaseRecord::finish() {

    if (_write_in_progress &&  _db) {
	sqlite3_exec( _db, "END TRANSACTION", NULL, NULL, NULL );
	if (sqlite3_finalize( _wrstmt )) 
	    cout <<"ERROR: couldn't finalize "<<_tablename<<": "
		 <<sqlite3_errmsg(_db)<<endl;
	_write_in_progress= false;
	cout <<"DEBUG: finished writing "<<_writecount<<" rows to '"
	     << _tablename << "'"  <<endl;
    }
    if (_read_in_progress && _db) {
	cout << "DEBUG: finalizing reading on '"<<_tablename<<"'"<<endl;
	if(sqlite3_finalize( _rdstmt ))
	    cout <<"ERROR: couldn't finalize "<<_tablename<<": "
		 <<sqlite3_errmsg(_db)<<endl;
	_read_in_progress=false;
    }

    
}


/**
 * You must have called prepareToRead() before calling this.
 * Thereafter, each time readFromDatabase is called, the mapped values
 * of your subclass will be updated with the next row of the database. 
 *
 * \returns 0 if no more rows are available, 1 if a row was read successfully.
 */
int
DatabaseRecord:: 
readFromDatabase() {

    std::map< std::string, DatabaseField >::iterator it;
    int i=0;
    int ret;

    if (_db == NULL) throw runtime_error("NO DATABASE CONNECTION!");

    ret = sqlite3_step(_rdstmt) ;
    if (ret == SQLITE_ROW) {

	for (it=_fieldmap.begin(); it != _fieldmap.end(); it++) {
	    switch (it->second.type) {
	    case FIELD_INT:
		*((int*)it->second.ptr) = sqlite3_column_int(_rdstmt,i);
		break;
	    case FIELD_DOUBLE:
		*((double*)it->second.ptr) = sqlite3_column_double(_rdstmt,i);
		break;
	    case FIELD_STRING:
		*((std::string*)(it->second.ptr))
		    =(const char*) sqlite3_column_text(_rdstmt,i);
		break;
	    }
	    i++;
	}
	sqlite3_reset(_wrstmt);
	return 1;
    }
    else if (ret==SQLITE_DONE) {
	return 0;
    }
    else{
	throw runtime_error(string("readFromDatabase() step: ")
			    +sqlite3_errmsg(_db));
	return 0;
    }

}


/**
 * Create the table in the database
 */
void DatabaseRecord::createTable() {
    
    string sql;

    if (_tablename == "")
	throw runtime_error("createTable: No table name specified");

    if (tableExists()) {
	cout << "WARNING: table '"<<_tablename<<"' already exists in datafile"
	     << ", dropping it..."<<endl;
	sql = "DROP TABLE "+_tablename;
	sqlite3_exec( _db, sql.c_str(), NULL,NULL,NULL );
    }


    sql = "CREATE TABLE "+_tablename+" ("+ getSchema() +")";
    
    int ret;

    cout <<"DEBUG: creating table: "<< _tablename << endl;

    ret = sqlite3_exec( _db, sql.c_str(), NULL, NULL, NULL );

    if (ret != SQLITE_OK) {
	throw runtime_error("createTable(): "+sql+": "+sqlite3_errmsg(_db));
    }

}


/**
 * Returns true if the table set by setTableName() exists in the database.
 */
bool DatabaseRecord::tableExists() {
    
    string sql = "SELECT * FROM "+_tablename+" LIMIT 1";

    if (_db == NULL) throw runtime_error("NO DATABASE CONNECTION!");

    if (sqlite3_exec( _db, sql.c_str(), NULL,NULL,NULL) ==SQLITE_OK)
	return true;
    else 
	return false;
    
}


/**
 * clear all the entries in the table (new writes are usually appended)
 */
void 
DatabaseRecord::
clearTable() {
    
    if (_db == NULL) throw runtime_error("NO DATABASE CONNECTION!");

    if (tableExists()) {
	cout << "DEBUG: clearing table '"<<_tablename<<"'"<<endl;
	string sql = "DELETE FROM "+_tablename;
	if (sqlite3_exec( _db, sql.c_str(), NULL,NULL,NULL ) != SQLITE_OK){
	    throw runtime_error("clearTable(): '"+sql+"': "
				+sqlite3_errmsg(_db));
	}
    }

}


/**
 * Returns the number of rows in the database.
 *
 * \param where: an SQL "where" clause.  If not specified, all rows are counted.
 */
int 
DatabaseRecord::
count(std::string where) {
    string sql = "SELECT count() FROM "+_tablename;
    sqlite3_stmt *stmt;
    int c;

    if (_db == NULL) throw runtime_error("NO DATABASE CONNECTION!");

    if (where!="") sql.append(" WHERE "+where);

    sqlite3_prepare(_db,sql.c_str(),sql.length(),&stmt,NULL);
    sqlite3_step(stmt);
    c=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    
    return c;

}

string join( std::string delim, std::vector< std::string > &strvect ) {
    int i;
    string str;

    if (strvect.size() == 1) { return strvect[0]; }
    for (i=0; i<strvect.size()-1; i++) {
	str.append(strvect[i]+delim);
    }
    str.append(strvect[i]);
    return str;
}


/**
 * Prints out all currently mapped values. Just helpful for debugging. 
 */
ostream&
DatabaseRecord::
print(ostream& stream) {
    
    std::map< std::string, DatabaseField >::iterator it;
    
    for (it=_fieldmap.begin(); it != _fieldmap.end(); it++) {
	stream << it->first << ":  ";
	switch (it->second.type) {
	case FIELD_INT:
	    stream << *((int*)it->second.ptr);
	    break;
	case FIELD_DOUBLE:
	    stream <<  *((double*)it->second.ptr);
	    break;
	case FIELD_STRING:
	    stream << "'"<<*((std::string*)it->second.ptr)<<"'";
	    break;
	}
	stream << endl;
    }
    return stream;
}


/**
 * Set all mapped values in the struct to zero
 */
void
DatabaseRecord::
zero() {
    std::map< std::string, DatabaseField >::iterator it;
	
    for (it=_fieldmap.begin(); it != _fieldmap.end(); it++) {
	switch (it->second.type) {
	case FIELD_INT:
	    *((int*)it->second.ptr) = 0;
	    break;
	case FIELD_DOUBLE:
	    *((double*)it->second.ptr) = 0.0;
	    break;
	case FIELD_STRING:
	    *((string*)it->second.ptr) = "";
	    break;
	}
    }
}

std::ostream& operator<<( std::ostream &stream,DatabaseRecord &rec ) {
    return rec.print(stream);
}
