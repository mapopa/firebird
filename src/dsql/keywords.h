/* This list is maintained in alphabetical sorted order by 2nd column.
   The following command will resort the list -- except for this comment
   sort -t , -k 2b,2b keywords.h 
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
   See dsql/parse.y for a chronological list. */
    {NOT_LSS, "!<", 1},
	{NEQ, "!=", 1},
	{NOT_GTR, "!>", 1},
	{LPAREN, "(", 1},
	{RPAREN, ")", 1}, 
    {COMMA, ",", 1}, 
    {LSS, "<", 1}, 
    {LEQ, "<=", 1}, 
    {NEQ, "<>", 1},	/* Alias of != */
	{EQL, "=", 1},
	{GTR, ">", 1},
	{GEQ, ">=", 1},
	{ACTION, "ACTION", 1},
	{ACTIVE, "ACTIVE", 1},
	{ADD, "ADD", 1},
	{ADMIN, "ADMIN", 1},
	{AFTER, "AFTER", 1},
	{ALL, "ALL", 1},
	{ALTER, "ALTER", 1}, 
    {AND, "AND", 1}, 
    {ANY, "ANY", 1}, 
    {AS, "AS", 1}, 
    {ASC, "ASC", 1},	/* Alias of ASCENDING */
	{ASC, "ASCENDING", 1},
	{AT, "AT", 1},
	{AUTO, "AUTO", 1},
	{AVG, "AVG", 1},
	{BASENAME, "BASE_NAME", 1},
	{BEFORE, "BEFORE", 1},
	{BEGIN, "BEGIN", 1},
	{BETWEEN, "BETWEEN", 1},
	{BLOB, "BLOB", 1},
	{BY, "BY", 1},
	{CACHE, "CACHE", 1},
	{CASCADE, "CASCADE", 1},
	{CAST, "CAST", 1},
	{KW_CHAR, "CHAR", 1},
	{CHARACTER, "CHARACTER", 1},
	{CHECK, "CHECK", 1},
	{CHECK_POINT_LEN, "CHECK_POINT_LENGTH", 1},
	{COLLATE, "COLLATE", 1},
	{COLUMN, "COLUMN", 2},
	{COMMIT, "COMMIT", 1},
	{COMMITTED, "COMMITTED", 1},
	{COMPUTED, "COMPUTED", 1},
	{CONDITIONAL, "CONDITIONAL", 1},
	{CONSTRAINT, "CONSTRAINT", 1},
	{CONTAINING, "CONTAINING", 1},
	{COUNT, "COUNT", 1},
	{CREATE, "CREATE", 1},
	{CSTRING, "CSTRING", 1},
	{CURRENT, "CURRENT", 1},
	{CURRENT_DATE, "CURRENT_DATE", 2},
	{CURRENT_TIME, "CURRENT_TIME", 2},
	{CURRENT_TIMESTAMP, "CURRENT_TIMESTAMP", 2},
	{CURSOR, "CURSOR", 1},
	{DATABASE, "DATABASE", 1},
	{DATE, "DATE", 1},
	{DAY, "DAY", 2},
	{DEBUG, "DEBUG", 1},
	{KW_DEC, "DEC", 1},
	{DECIMAL, "DECIMAL", 1},
	{DECLARE, "DECLARE", 1},
	{DEFAULT, "DEFAULT", 1}, 
    {DELETE, "DELETE", 1}, 
    {DESC, "DESC", 1},	/* Alias of DESCENDING */
	{DESC, "DESCENDING", 1},
	{DISTINCT, "DISTINCT", 1},
	{DO, "DO", 1},
	{DOMAIN, "DOMAIN", 1},
	{KW_DOUBLE, "DOUBLE", 1},
	{DROP, "DROP", 1},
	{ELSE, "ELSE", 1},
	{END, "END", 1},
	{ENTRY_POINT, "ENTRY_POINT", 1},
	{ESCAPE, "ESCAPE", 1},
	{EXCEPTION, "EXCEPTION", 1},
	{EXECUTE, "EXECUTE", 1},
	{EXISTS, "EXISTS", 1},
	{EXIT, "EXIT", 1},
	{EXTERNAL, "EXTERNAL", 1},
	{EXTRACT, "EXTRACT", 2},
	{KW_FILE, "FILE", 1},
	{FILTER, "FILTER", 1},
	{KW_FLOAT, "FLOAT", 1},
	{FOR, "FOR", 1},
	{FOREIGN, "FOREIGN", 1},
	{FREE_IT, "FREE_IT", 1},
	{FROM, "FROM", 1},
	{FULL, "FULL", 1},
	{FUNCTION, "FUNCTION", 1},
	{GDSCODE, "GDSCODE", 1},
	{GENERATOR, "GENERATOR", 1},
	{GEN_ID, "GEN_ID", 1},
	{GRANT, "GRANT", 1},
	{GROUP, "GROUP", 1},
	{GROUP_COMMIT_WAIT, "GROUP_COMMIT_WAIT_TIME", 1},
	{HAVING, "HAVING", 1},
	{HOUR, "HOUR", 2},
	{IF, "IF", 1},
	{IN, "IN", 1},
	{INACTIVE, "INACTIVE", 1},
	{INDEX, "INDEX", 1},
	{INNER, "INNER", 1},
	{INPUT_TYPE, "INPUT_TYPE", 1},
	{INSERT, "INSERT", 1},
	{KW_INT, "INT", 1},
	{INTEGER, "INTEGER", 1},
	{INTO, "INTO", 1},
	{IS, "IS", 1},
	{ISOLATION, "ISOLATION", 1},
	{JOIN, "JOIN", 1},
	{KEY, "KEY", 1},
	{LEFT, "LEFT", 1},
	{LENGTH, "LENGTH", 1},
	{LEVEL, "LEVEL", 1},
	{LIKE, "LIKE", 1},
	{LOGFILE, "LOGFILE", 1},
	{LOG_BUF_SIZE, "LOG_BUFFER_SIZE", 1},
	{KW_LONG, "LONG", 1},
	{MANUAL, "MANUAL", 1},
	{MAXIMUM, "MAX", 1},
	{MAX_SEGMENT, "MAXIMUM_SEGMENT", 1},
	{MERGE, "MERGE", 1},
	{MESSAGE, "MESSAGE", 1},
	{MINIMUM, "MIN", 1},
	{MINUTE, "MINUTE", 2},
	{MODULE_NAME, "MODULE_NAME", 1},
	{MONTH, "MONTH", 2},
	{NAMES, "NAMES", 1},
	{NATIONAL, "NATIONAL", 1},
	{NATURAL, "NATURAL", 1},
	{NCHAR, "NCHAR", 1},
	{NO, "NO", 1},
	{NOT, "NOT", 1},
	{KW_NULL, "NULL", 1},
	{KW_NUMERIC, "NUMERIC", 1},
	{NUM_LOG_BUFS, "NUM_LOG_BUFFERS", 1},
	{OF, "OF", 1},
	{ON, "ON", 1},
	{ONLY, "ONLY", 1},
	{OPTION, "OPTION", 1},
	{OR, "OR", 1},
	{ORDER, "ORDER", 1},
	{OUTER, "OUTER", 1},
	{OUTPUT_TYPE, "OUTPUT_TYPE", 1},
	{OVERFLOW, "OVERFLOW", 1},
	{PAGE, "PAGE", 1},
	{PAGES, "PAGES", 1},
	{PAGE_SIZE, "PAGE_SIZE", 1},
	{PARAMETER, "PARAMETER", 1},
	{PASSWORD, "PASSWORD", 1},
	{PLAN, "PLAN", 1},
	{POSITION, "POSITION", 1},
	{POST_EVENT, "POST_EVENT", 1},
	{PRECISION, "PRECISION", 1},
	{PRIMARY, "PRIMARY", 1},
	{PRIVILEGES, "PRIVILEGES", 1},
	{PROCEDURE, "PROCEDURE", 1},
	{PROTECTED, "PROTECTED", 1},
	{RAW_PARTITIONS, "RAW_PARTITIONS", 1},
	{DB_KEY, "RDB$DB_KEY", 1},
	{READ, "READ", 1},
	{REAL, "REAL", 1},
	{VERSION, "RECORD_VERSION", 1},
	{REFERENCES, "REFERENCES", 1}, 
    {RESERVING, "RESERV", 1},	/* Alias of RESERVING */
	{RESERVING, "RESERVING", 1},
	{RESTRICT, "RESTRICT", 1},
	{RETAIN, "RETAIN", 1},
	{RETURNING_VALUES, "RETURNING_VALUES", 1},
	{RETURNS, "RETURNS", 1},
	{REVOKE, "REVOKE", 1},
	{RIGHT, "RIGHT", 1},
	{ROLE, "ROLE", 1}, 
    {ROLLBACK, "ROLLBACK", 1}, 
    {DATABASE, "SCHEMA", 1},	/* Alias of DATABASE */
	{SECOND, "SECOND", 2},
	{SEGMENT, "SEGMENT", 1},
	{SELECT, "SELECT", 1},
	{SET, "SET", 1},
	{SHADOW, "SHADOW", 1},
	{SHARED, "SHARED", 1},
	{SINGULAR, "SINGULAR", 1},
	{SIZE, "SIZE", 1},
	{SMALLINT, "SMALLINT", 1},
	{SNAPSHOT, "SNAPSHOT", 1},
	{SOME, "SOME", 1},
	{SORT, "SORT", 1},
	{SQLCODE, "SQLCODE", 1},
	{STABILITY, "STABILITY", 1}, 
    {STARTING, "STARTING", 1}, 
    {STARTING, "STARTS", 1},	/* Alias of STARTING */
	{STATISTICS, "STATISTICS", 1},
	{SUB_TYPE, "SUB_TYPE", 1},
	{SUM, "SUM", 1},
	{SUSPEND, "SUSPEND", 1},
	{TABLE, "TABLE", 1},
	{THEN, "THEN", 1},
	{TIME, "TIME", 2},
	{TIMESTAMP, "TIMESTAMP", 2},
	{TO, "TO", 1},
	{TRANSACTION, "TRANSACTION", 1},
	{TRIGGER, "TRIGGER", 1},
	{TYPE, "TYPE", 2},
	{UNCOMMITTED, "UNCOMMITTED", 1},
	{UNION, "UNION", 1},
	{UNIQUE, "UNIQUE", 1},
	{UPDATE, "UPDATE", 1},
	{KW_UPPER, "UPPER", 1},
	{USER, "USER", 1},
	{KW_VALUE, "VALUE", 1},
	{VALUES, "VALUES", 1},
	{VARCHAR, "VARCHAR", 1},
	{VARIABLE, "VARIABLE", 1},
	{VARYING, "VARYING", 1},
	{VIEW, "VIEW", 1},
	{WAIT, "WAIT", 1},
	{WEEKDAY, "WEEKDAY", 2},
	{WHEN, "WHEN", 1},
	{WHERE, "WHERE", 1},
	{WHILE, "WHILE", 1},
	{WITH, "WITH", 1},
	{WORK, "WORK", 1},
	{WRITE, "WRITE", 1}, 
    {YEAR, "YEAR", 2}, 
    {YEARDAY, "YEARDAY", 2}, 
    {NOT_LSS, "^<", 1},	/* Alias of !< */
	{NEQ, "^=", 1},				/* Alias of != */
	{NOT_GTR, "^>", 1},			/* Alias of !> */
	{CONCATENATE, "||", 1}, 
    {NOT_LSS, "~<", 1},	/* Alias of !< */
	{NEQ, "~=", 1},				/* Alias of != */
	{NOT_GTR, "~>", 1},			/* Alias of !> */
