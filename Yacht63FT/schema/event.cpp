#include "event.hpp"

#include "dbsystem.hpp"
#include "dbtypes.hpp"

#include "dbmisc.hpp"

using namespace WarGrey::SCADA;

static const char* event_rowids[] = { "uuid" };

static TableColumnInfo event_columns[] = {
    { "uuid", SDT::Integer, nullptr, DB_PRIMARY_KEY | 0 | 0 },
    { "type", SDT::Text, nullptr, 0 | DB_NOT_NULL | 0 },
    { "name", SDT::Text, nullptr, 0 | DB_NOT_NULL | DB_UNIQUE },
    { "ctime", SDT::Integer, nullptr, 0 | 0 | 0 },
    { "mtime", SDT::Integer, nullptr, 0 | 0 | 0 },
};

/**************************************************************************************************/
AlarmEvent WarGrey::SCADA::make_event() {
    AlarmEvent self;

    default_event(self);

    return self;
}

void WarGrey::SCADA::default_event(AlarmEvent& self) {
    self.uuid = pk64_timestamp();
    self.type = "table";
    self.ctime = current_milliseconds();
    self.mtime = current_milliseconds();
}

void WarGrey::SCADA::refresh_event(AlarmEvent& self) {
    self.mtime = current_milliseconds();
}

void WarGrey::SCADA::store_event(AlarmEvent& self, IPreparedStatement* stmt) {
    stmt->bind_parameter(0, self.uuid);
    stmt->bind_parameter(1, self.type);
    stmt->bind_parameter(2, self.name);
    stmt->bind_parameter(3, self.ctime);
    stmt->bind_parameter(4, self.mtime);
}

void WarGrey::SCADA::restore_event(AlarmEvent& self, IPreparedStatement* stmt) {
    self.uuid = stmt->column_int64(0U);
    self.type = stmt->column_text(1U);
    self.name = stmt->column_text(2U);
    self.ctime = stmt->column_maybe_int64(3U);
    self.mtime = stmt->column_maybe_int64(4U);
}

/**************************************************************************************************/
void WarGrey::SCADA::create_event(IDBSystem* dbc, bool if_not_exists) {
    IVirtualSQL* vsql = dbc->make_sql_factory(event_columns, sizeof(event_columns)/sizeof(TableColumnInfo));
    std::string sql = vsql->create_table("event", event_rowids, sizeof(event_rowids)/sizeof(std::string), if_not_exists);

    dbc->exec(sql);
}

void WarGrey::SCADA::insert_event(IDBSystem* dbc, AlarmEvent* self, bool replace) {
    insert_event(dbc, self, 1, replace);
}

void WarGrey::SCADA::insert_event(IDBSystem* dbc, AlarmEvent* selves, size_t count, bool replace) {
    IVirtualSQL* vsql = dbc->make_sql_factory(event_columns, sizeof(event_columns)/sizeof(TableColumnInfo));
    std::string sql = vsql->insert_into("event", replace);
    IPreparedStatement* stmt = dbc->prepare(sql);

    if (stmt != nullptr) {
        for (int i = 0; i < count; i ++) {
            store_event(selves[i], stmt);
            dbc->exec(stmt);
            stmt->reset(true);
        }

        delete stmt;
    }
}

std::list<AlarmEvent> WarGrey::SCADA::select_event(IDBSystem* dbc, unsigned int limit, unsigned int offset) {
    IVirtualSQL* vsql = dbc->make_sql_factory(event_columns, sizeof(event_columns)/sizeof(TableColumnInfo));
    std::string sql = vsql->select_from("event", limit, offset);
    IPreparedStatement* stmt = dbc->prepare(sql);
    std::list<AlarmEvent> queries;

    if (stmt != nullptr) {
        AlarmEvent self;

        while(stmt->step()) {
            restore_event(self, stmt);
            queries.push_back(self);
        }

        delete stmt;
    }

    return queries;
}

void WarGrey::SCADA::drop_event(IDBSystem* dbc) {
    IVirtualSQL* vsql = dbc->make_sql_factory(event_columns, sizeof(event_columns)/sizeof(TableColumnInfo));
    std::string sql = vsql->drop_table("event");

    dbc->exec(sql);
}

