// creation of structures

var sections = ["Stand-alone configuration", "Groups", "Pairs"];
var wndPop = null;


var tables = {
	"t_se": [
		{"name":"SE name", "dbname":"name", "ord":"asc", "pk":"Y"},
		{"name":"State", "dbname":"state"},
		{"name":"Connections", "dbname":"connections", "hide":"Y", "link":{"table":"t_connection", "db_keys":["symbolicname"], "display":"table"}}
	],
	"t_connection": [
		{"name":"Source", "dbname":"source", "pk":"Y"},
		{"name":"Destination", "dbname":"destination", "pk":"Y"},
		{"name":"Link state", "dbname":"lstate"},
		{"name":"Link", "dbname":"symbolicname", "ord":"asc"},
		{"name":"Number of streams", "dbname":"nostreams"},
		{"name":"TCP buffer size", "dbname":"tcp_buffer_size"},
		{"name":"Timeout","dbname":"urlcopy_tx_to"},
		{"name":"No activity timeout", "dbname":"no_tx_activity_to"},
		{"name":"Share", "dbname":"share", "hide":"Y", "link":{"table":"t_config", "db_keys":["source", "destination"], "display":"table"}}
	],
	"t_config" : [
		{"name":"Source", "dbname":"source", "hide":"Y"},
		{"name":"Destination", "dbname":"destination","hide":"Y"},
		{"name":"VO", "dbname":"vo", "ord":"asc", "pk":"Y"},
		{"name":"Active", "dbname":"active"}
	],
	"t_config_pair" : [
		{"name":"Source", "dbname":"source", "ord":"asc", "pk":"Y"},
		{"name":"Destination", "dbname":"destination", "ord":"asc", "pk":"Y"},
		{"name":"Name", "dbname":"symbolicname", "ord":"asc"},
		{"name":"State", "dbname":"state"},
		{"name":"Number of streams", "dbname":"nostreams"},
		{"name":"TCP buffer size", "dbname":"tcp_buffer_size"},
		{"name":"Timeout", "dbname":"urlcopy_tx_to"},
		{"name":"No activity timeout", "dbname":"no_tx_activity_to"},
		{"name":"Share", "dbname":"share", "hide":"Y", "link":{"table":"t_config", "db_keys":["source", "destination"], "display":"table"}}
	],
	"t_se_groups" : [
		{"name":"Group", "dbname":"groupname", "pk":"Y", "ord":"asc"},
		{"name":"Members", "dbname":"members", "hide":"Y", "link":{"table":"t_group_member", "db_keys":["groupname"], "display":"table"}}
	],
	"t_group_member" : [
		{"name":"Group", "dbname":"groupname", "hide":"Y"},
		{"name":"Member", "dbname":"member", "pk":"Y", "ord":"asc"}
	]
};

var queries = {
	"t_se" : "select * " +
		"from ( " +
			"select  t1.name name, t1.state state,  t2.symbolicName symbolicname, t2.source source, t2.destination destination, t2.state lstate," + 
				"nostreams, tcp_buffer_size, urlcopy_tx_to, no_tx_activity_to, " +
				"vo, active  " +
			"from t_se t1, t_link_config t2, " +
				"(select symbolicName, t3.source source, t3.destination destination, vo, active " +
				"from (select distinct symbolicName, source, destination " +
					"from t_link_config " +
					"where source='*'  or destination='*' " +
					")  t3, t_share_config t4 " +
				"where t3.source=t4.source(+) and t3.destination=t4.destination(+)) t5 " +
			"where ((t1.name=t2.source and t2.destination='*')  or (t1.name=t2.destination and t2.source='*'))  " +
				"and t2.source=t5.source and t2.destination=t5.destination " +
			"union " +
			"select  t11.name, t11.state, null symbolicname, null source, null destination, null lstate," + 
				"null nostreams, null tcp_buffer_size, null urlcopy_tx_to, null no_tx_activity_to, " +
				"null vo, null active  " +
			"from t_se t11 " +
			"where not exists (select * from t_link_config t12 where " +
				"(t12.source=t11.name and t12.destination='*') or (t12.source='*' and t12.destination=t11.name))" +
		")",
	"t_se_groups" : "select groupname, member from t_group_members ",
	"t_groups_all" : "select distinct groupname from t_group_members ",
	"t_group_new" : "select distinct t1.groupname name " +
			"from t_group_members t1 " +
			"where t1.groupname not in (select t2.name from t_se t2) ",
	"t_config_pair" : "select * " +
			"from ( " +
				"select  t1.source source, t1.destination destination, symbolicname, state, " + 
					"nostreams, tcp_buffer_size, urlcopy_tx_to, no_tx_activity_to, " +
					"vo, active  " +
				"from t_link_config t1, t_share_config t2 " +
				"where t1.source != '*'  and t1.destination != '*' " +
				"and t1.source=t2.source(+) and t1.destination=t2.destination(+) " +
			")",
	"t_se_all" : "select name from t_se "
			
};


var sorts = {
	"t_se" : " name desc, symbolicname, vo ",
	"t_se_groups" : " groupname desc, member ",
	"t_groups_all" : " groupname ",
	"t_group_new" : " name ",
	"t_config_pair" : " source desc, destination desc, vo ",
	"t_se_all" : " name "
}
