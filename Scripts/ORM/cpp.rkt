#lang racket

(provide (all-defined-out))

(define cstring 'std::string)

(define &linebreak
  (lambda [[count 1]]
    (let nbsp ([c count])
      (when (> c 0)
        (newline)
        (nbsp (sub1 c))))))

(define &hspace
  (lambda [[count 1]]
    (let hsp ([c count])
      (when (> c 0)
        (display #\space)
        (hsp (sub1 c))))))

(define &htab
  (lambda [[count 1]]
    (&hspace (* count 4))))

(define &brace
  (lambda [indent #:semicolon? [semicolon? #false]]
    (&htab indent)
    (display #\})

    (when (and semicolon?)
      (display #\;))
    (&linebreak)))

(define &pragma
  (lambda pragmas
    (for ([pragma (in-list pragmas)])
      (printf "#pragma ~a~n" pragma))
    (when (> (length pragmas) 0)
      (&linebreak 1))))

(define &include
  (lambda headers
    (for ([header (in-list headers)])
      (cond [(string? header) (printf "#include ~s~n" header)]
            [else (printf "#include <~a>~n" header)]))
    (when (> (length headers) 0)
      (&linebreak 1))))

(define &namespace
  (lambda [ns λbody]
    (printf "namespace ~a {~n" ns)
    (λbody 1)
    (&brace 0)))

(define &using-namespace
  (lambda namespaces
    (for ([ns (in-list namespaces)])
      (printf "using namespace ~a;~n" ns))
    (&linebreak 1)))

(define &primary-key
  (lambda [Table_pk rowids idtypes indent]
    (cond [(> (length rowids) 1) (&struct Table_pk rowids idtypes indent)]
          [else (&htab indent)
                (printf "typedef ~a ~a;~n" (car idtypes) Table_pk)
                (&linebreak 1)])))

(define &struct
  (lambda [name fields types indent]
    (&htab indent)
    (printf "private struct ~a {~n" name)

    (for ([field (in-list fields)]
          [type (in-list types)])
      (&htab (add1 indent))
      (printf "~a ~a;~n" type field))
    
    (&brace indent #:semicolon? #true)
    (&linebreak 1)))

(define &table-column-info
  (lambda [var_columns var_rowids rowids cols dbtypes not-nulls uniques]
    (printf "static const char* ~a[] = { ~a };~n" var_rowids
            (let strcat ([s (format "~s" (symbol->string (car rowids)))]
                         [r (cdr rowids)])
              (cond [(null? r) s]
                    [else (strcat (format "~a, ~s" s (symbol->string (car r)))
                                  (cdr r))])))
    (&linebreak 1)
    
    (printf "static TableColumnInfo ~a[] = {~n" var_columns)
    (for ([col (in-list cols)]
          [type (in-list dbtypes)]
          [nnil (in-list not-nulls)]
          [uniq (in-list uniques)])
      (&hspace 4)
      (printf "{ ~s, SDT::~a, nullptr, ~a | ~a | ~a },~n"
              (symbol->string col)
              (symbol->string type)
              (if (memq col rowids) 'DB_PRIMARY_KEY 0)
              (if (and nnil) 'DB_NOT_NULL 0)
              (if (and uniq) 'DB_UNIQUE 0)))
    (&brace 0 #:semicolon? #true)
    (&linebreak 1)))

(define &make-table
  (lambda [λname classname indent/default]
    (cond [(number? indent/default)
           (&htab indent/default) (printf "WarGrey::SCADA::~a ~a();~n" classname λname)]
          [else (printf "~a WarGrey::SCADA::~a() {~n" classname λname)
                (&htab 1) (printf "~a self;~n" classname)
                (&linebreak 1)
                (&htab 1) (printf "~a(self);~n" indent/default)
                (&linebreak 1)
                (&htab 1) (printf "return self;~n")
                (&brace 0)
                (&linebreak 1)])))

(define &default-table
  (case-lambda
    [(λname classname indent)
     (&htab indent) (printf "void ~a(WarGrey::SCADA::~a& self);~n" λname classname)]
    [(λname classname fields defvals)
     (printf "void WarGrey::SCADA::~a(~a& self) {~n" λname classname)
     (for ([field (in-list fields)]
           [val (in-list defvals)])
       (when (and val)
         (&htab 1) (printf "self.~a = " field)
         (cond [(symbol? val) (printf "~a();~n" val)]
               [(string? val) (printf "~s;~n" val)]
               [else (printf "~a;~n" val)])))
     (&brace 0)
     (&linebreak 1)]))

(define &refresh-table
  (case-lambda
    [(λname classname indent)
     (&htab indent) (printf "void ~a(WarGrey::SCADA::~a& self);~n" λname classname)]
    [(λname classname fields autovals)
     (printf "void WarGrey::SCADA::~a(~a& self) {~n" λname classname)
     (for ([field (in-list fields)]
           [val (in-list autovals)])
       (when (and val)
         (&htab 1) (printf "self.~a = ~a();~n" field val)))
     (&brace 0)
     (&linebreak 1)]))

(define &store-table
  (lambda [λname classname indent/fields]
    (cond [(number? indent/fields)
           (&htab indent/fields) (printf "void ~a(WarGrey::SCADA::~a& self, WarGrey::SCADA::IPreparedStatement* stmt);~n" λname classname)]
          [else (printf "void WarGrey::SCADA::~a(~a& self, IPreparedStatement* stmt) {~n" λname classname)
                (for ([field (in-list indent/fields)]
                      [idx (in-naturals)])
                  (&htab 1) (printf "stmt->bind_parameter(~a, self.~a);~n" idx field))
                (&brace 0)
                (&linebreak 1)])))

(define &restore-table
  (case-lambda
    [(λname classname indent)
     (&htab indent) (printf "void ~a(WarGrey::SCADA::~a& self, WarGrey::SCADA::IPreparedStatement* stmt);~n" λname classname)]
    [(λname classname fields types not-nulls rowids)
     (printf "void WarGrey::SCADA::~a(~a& self, IPreparedStatement* stmt) {~n" λname classname)
     (for ([field (in-list fields)]
           [type (in-list types)]
           [n-nil (in-list not-nulls)]
           [idx (in-naturals)])
       (define sqlite-types
         (case type
           [(Integer) 'int64]
           [(Float) 'double]
           [else 'text]))
       (&htab 1) (printf "self.~a = stmt->column~a~a(~aU);~n" field (if (or (memq field rowids) n-nil) "_" "_maybe_") sqlite-types idx))
     (&brace 0)
     (&linebreak 1)]))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define &create-table
  (case-lambda
    [(λname indent)
     (&htab indent) (printf "void ~a(WarGrey::SCADA::IDBSystem* dbc, bool if_not_exists = true);~n" λname)]
    [(λname tablename column_infos table_rowids)
     (printf "void WarGrey::SCADA::~a(IDBSystem* dbc, bool if_not_exists) {~n" λname)
     (&htab 1) (printf "IVirtualSQL* vsql = dbc->make_sql_factory(~a, sizeof(~a)/sizeof(TableColumnInfo));~n" column_infos column_infos)
     (&htab 1) (printf "~a sql = vsql->create_table(~s, ~a, sizeof(~a)/sizeof(~a), if_not_exists);~n" cstring
                       (symbol->string tablename) table_rowids table_rowids cstring)
     (&linebreak)
     (&htab 1) (printf "dbc->exec(sql);~n")
     (&brace 0)
     (&linebreak 1)]))

(define &insert-table
  (case-lambda
    [(λname classname indent)
     (&htab indent) (printf "void ~a(WarGrey::SCADA::IDBSystem* dbc, ~a* self, bool replace = false);~n" λname classname)
     (&htab indent) (printf "void ~a(WarGrey::SCADA::IDBSystem* dbc, ~a* selves, int count, bool replace = false);~n" λname classname)]
    [(λname classname tablename store column_infos)
     (printf "void WarGrey::SCADA::~a(IDBSystem* dbc, ~a* self, bool replace) {~n" λname classname)
     (&htab 1) (printf "~a(dbc, self, 1, replace);~n" λname)
     (&brace 0)
     (&linebreak 1)
     (printf "void WarGrey::SCADA::~a(IDBSystem* dbc, ~a* selves, int count, bool replace) {~n" λname classname)
     (&htab 1) (printf "IVirtualSQL* vsql = dbc->make_sql_factory(~a, sizeof(~a)/sizeof(TableColumnInfo));~n" column_infos column_infos)
     (&htab 1) (printf "~a sql = vsql->insert_into(~s, replace);~n" cstring (symbol->string tablename))
     (&htab 1) (printf "IPreparedStatement* stmt = dbc->prepare(sql);~n")
     (&linebreak 1)
     (&htab 1) (printf "if (stmt != nullptr) {~n")
     (&htab 2) (printf "for (int i = 0; i < count; i ++) {~n")
     (&htab 3) (printf "~a(selves[i], stmt);~n" store)
     (&htab 3) (printf "dbc->exec(stmt);~n")
     (&htab 3) (printf "stmt->reset(true);~n")
     (&brace 2)
     (&linebreak 1)
     (&htab 2) (printf "delete stmt;~n")
     (&brace 1)
     (&brace 0)
     (&linebreak 1)]))

(define &select-table
  (case-lambda
    [(λname classname indent)
     (&htab indent) (printf "std::list<~a> ~a(WarGrey::SCADA::IDBSystem* dbc, unsigned int limit = 0, unsigned int offset = 0);~n" classname λname)]
    [(λname classname tablename restore column_infos)
     (printf "std::list<~a> WarGrey::SCADA::~a(IDBSystem* dbc, unsigned int limit, unsigned int offset) {~n" classname λname)
     (&htab 1) (printf "IVirtualSQL* vsql = dbc->make_sql_factory(~a, sizeof(~a)/sizeof(TableColumnInfo));~n" column_infos column_infos)
     (&htab 1) (printf "~a sql = vsql->select_from(~s, limit, offset);~n" cstring (symbol->string tablename))
     (&htab 1) (printf "IPreparedStatement* stmt = dbc->prepare(sql);~n")
     (&htab 1) (printf "std::list<~a> queries;~n" classname)
     (&linebreak 1)
     (&htab 1) (printf "if (stmt != nullptr) {~n")
     (&htab 2) (printf "~a self;~n" classname)
     (&linebreak 1)
     (&htab 2) (printf "while(stmt->step()) {~n")
     (&htab 3) (printf "~a(self, stmt);~n" restore)
     (&htab 3) (printf "queries.push_back(self);~n")
     (&brace 2)
     (&linebreak 1)
     (&htab 2) (printf "delete stmt;~n")
     (&brace 1)
     (&linebreak 1)
     (&htab 1) (printf "return queries;~n")
     (&brace 0)
     (&linebreak 1)]))

(define &drop-table
  (case-lambda
    [(λname indent)
     (&htab indent) (printf "void ~a(WarGrey::SCADA::IDBSystem* dbc);~n" λname)]
    [(λname tablename column_infos)
     (printf "void WarGrey::SCADA::~a(IDBSystem* dbc) {~n" λname)
     (&htab 1) (printf "IVirtualSQL* vsql = dbc->make_sql_factory(~a, sizeof(~a)/sizeof(TableColumnInfo));~n" column_infos column_infos)
     (&htab 1) (printf "~a sql = vsql->drop_table(~s);~n" cstring (symbol->string tablename))
     (&linebreak)
     (&htab 1) (printf "dbc->exec(sql);~n")
     (&brace 0)
     (&linebreak 1)]))