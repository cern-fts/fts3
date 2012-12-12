
/*
v1.03 Copyright (c) 2006 Stuart Colville                                                                                                    
http://muffinresearch.co.uk/archives/2006/04/29/getelementsbyclassname-deluxe-edition/
                                                                                                                                            
The function has three parameters:
strClass:
         string containing the class(es) that you are looking for
strTag (optional, defaults to '*') :
         An optional tag name to narrow the search to specific tags e.g. 'a' for links.
objContElm (optional, defaults to document):
         An optional object container to search inside. Again this narrows the scope of the search
*/                                                                                                                                          
                                                                                                                                            
function getElementsByClassName(strClass, strTag, objContElm) {                                                                             
  strTag = strTag || "*";
  objContElm = objContElm || document;
  var objColl = objContElm.getElementsByTagName(strTag);
  if (!objColl.length &&  strTag == "*" &&  objContElm.all) objColl = objContElm.all;
  var arr = new Array();
  var delim = strClass.indexOf('|') != -1  ? '|' : ' ';
  var arrClass = strClass.split(delim);
  for (var i = 0, j = objColl.length; i < j; i++) {
    var arrObjClass = objColl[i].className.split(' ');
    if (delim == ' ' && arrClass.length > arrObjClass.length) continue;
    var c = 0;
    comparisonLoop:
    for (var k = 0, l = arrObjClass.length; k < l; k++) {
      for (var m = 0, n = arrClass.length; m < n; m++) {
        if (arrClass[m] == arrObjClass[k]) c++;
        if ((delim == '|' && c == 1) || (delim == ' ' && c == arrClass.length)) {
          arr.push(objColl[i]);
          break comparisonLoop;
        }
      }
    }
  }
  return arr;
}   


function proc_header (row, cfg) {
	for (var i=0; i<cfg.length;i++) {
		if (cfg[i].link) {
			if (cfg[i].link.display == "table") {
				proc_header(row, tables[cfg[i].link.table]);
			} else {
			}
		} else {
			if (cfg[i].hide == "Y") continue;
//			if (cfg[i].ord) this.ord.push(cfg[i].ord);
			tcell = row.insertCell(-1);
			tcell.innerHTML = cfg[i].name;
		}
	}
	return;
}



function proc_row (tb, cfg, rec, lvl, set) {
	var i, j, k;
	var ret = {};

	var ln = cfg[cfg.length-1];
	if (ln["link"]) {
		var cfg_in = tables[ln["link"]["table"]];
		var rec_in = rec[ln["dbname"]];
		var arr={};
		var cnt = 0;
		for (k=rec_in.length-1; k>=0; k--) {
			arr = proc_row(tb, cfg_in, rec_in[k], lvl+1, set);
			cnt += arr.span;
		}
		for (j=cfg.length-2; j>=0; j--) {
			if (cfg[j].hide == "Y") continue;
			var tcell = arr.trow.insertCell(0);
			tcell.rowSpan = cnt;
if (set) tcell.style.backgroundColor = "#FF6600";
			var spn = document.createElement("SPAN");
			tcell.appendChild(spn);
			spn.innerHTML = rec[cfg[j].dbname];
			if (lvl == 1 && cfg[j].pk == "Y") {
				spn.className = "primary";
				spn.name = cfg[j].dbname;
			}
		}
		ret.span = cnt;
		ret.trow = arr.trow;
		return ret;
	} else {
		var trow, tcell;
		trow = tb.insertRow(0);
		for (var j=cfg.length-1; j>=0; j--) {
			if (cfg[j].hide == "Y") continue;
			tcell = trow.insertCell(0);
if (set) tcell.style.backgroundColor = "#FF6600";
			var spn = document.createElement("SPAN");
			tcell.appendChild(spn);
			spn.innerHTML = rec[cfg[j].dbname];
		}
		ret.span = 1;
		ret.trow = trow;
		return ret;
	}
}


function procStruct (type, parent, force, prev, data){
	var i;
	var force_in = force;
	var pks = [];
	for (i=0; i<tables[type].length; i++ ) {
		if (tables[type][i]["pk"]) pks.push(tables[type][i]["dbname"]);
	}

	for (i=0; i<pks.length; i++) {
		if (prev[pks[i]] && data[pks[i]] == prev[pks[i]]) continue;
		force_in = 1;
		break;
	}
	var rec = {};
	var fin = 1;

	for (i=0; i<tables[type].length; i++) {
		var rc = tables[type][i];
		if (rc["link"]) {
			fin = 0;
			var nx = {};
			for (var p in rc["link"]) {
				nx[p] = rc["link"][p];
			}
			var nnx = rc["dbname"];
			if (force_in) {
				rec[nnx] = [];
				parent.push(rec);
			}
			parent = parent[parent.length-1][nnx];
			procStruct(nx["table"], parent, force_in, prev, data);
		} else {
			if (rc["hide"]) continue;
			if (force_in) rec[rc["dbname"]] = data[rc["dbname"]];
		}
	}
	if (fin) parent.push(rec);
	return;
}

function sel_table (type, div, width, htm, w, h) {

	var trow, tcell, i, j;
	var dv,ln;
	var self = this;  

	this.type = type;
	var l = div;
	if(typeof div == "string"){
		l = document.getElementById(div);
	}
	this.div = l;
	
	dv = document.createElement("DIV");
	dv.style.padding = "10px"
         this.div.appendChild(dv);

	ln = document.createElement("BUTTON");
	ln.style.width = "100px";
	dv.appendChild(ln);
	ln.appendChild(document.createTextNode("Create new"));
	ln.onclick = function () {self.insertData()};

	ln = document.createElement("BUTTON");
	ln.style.width = "100px";
	dv.appendChild(ln);
	ln.appendChild(document.createTextNode("Refresh"));
	ln.onclick = function () {self.refreshData()};

	var tbl = document.createElement("TABLE");
	tbl.className = "sort-table";
	tbl.id = this.type;
	tbl.style.width = width;
	var thd = document.createElement("THEAD");
	var tbd = document.createElement("TBODY");

	l.appendChild(tbl);
	tbl.appendChild(thd);
	tbl.appendChild(tbd);

	this.tdom = tbl;
	this.thead = this.tdom.tHead;
	this.tbody = this.tdom.tBodies[0];
	
	this.cfg = tables[type];

// deal with sorting for select not used yet!! 
	this.ord = [];

	var pks = [];
	for (i=0; i<tables[type].length; i++ ) {
		if (tables[type][i]["pk"]) pks.push(tables[type][i]["dbname"]);
	}
	this.primary = pks;

	trow = this.thead.insertRow(-1);
	proc_header(trow, this.cfg);
	tcell = trow.insertCell(-1);
	tcell.innerHTML = "&nbsp";

	this.htm = htm;
	this.w = w;
	this.h = h;
	return;
}

sel_table.prototype.drawTable = function (arr) {
	this.clearTable();
	var crow = 0;
//	for (var i=arr.length-1; i>=0; i++) {
	for (var i=0; i<arr.length; i++) {
		crow += this.insRow(arr[i], crow);
	}
	return;
}

sel_table.prototype.clearTable = function () {
	while(this.tbody.rows.length > 0){
		this.tbody.deleteRow(0);
	}                                                                                                            
	return true;                                                                           

}

sel_table.prototype.delRow = function (ind, len) {
//alert("ind = " + ind + " len = " + len);
	var j = this.tbody.rows.length - ind - len;
	if (this.tbody.rows[j]){
		for (var i = j; i< +len+j; i++) {
//var tt = +len+j;
//alert("j = " + j + " j+len = " + tt + " i = " + i);
			this.tbody.deleteRow(j);
		}
		return true;
	}                                                                                                            
	return false;                                                                           
}

sel_table.prototype.insRow = function (rec, ind) {
	var tcell;
	var self = this;
	var set = false;

// specially for empty connections for SE
if (this.type == "t_se" && !rec.connections[0].source) set = true;
	var ret = proc_row(this.tbody, this.cfg, rec, 1, set);
// add command buttons
	tcell = ret.trow.insertCell(-1);
	tcell.rowSpan = ret.span;
	tcell.style.width = "0px";
	var bEdit = document.createElement("BUTTON");
	bEdit.style.width = "100px";
	tcell.appendChild(bEdit);
	bEdit.appendChild(document.createTextNode(" Edit "));
	tcell.appendChild(document.createElement("BR"));  
	bEdit.onclick = function (k, m) {return function(){self.modData(k, m)}}(ind, ret.span);
                
	var bDelete = document.createElement("BUTTON");
	bDelete.style.width = "100px";
	tcell.appendChild(bDelete);
	bDelete.appendChild(document.createTextNode("Delete"));                             
	bDelete.onclick = function (k, m) {return function(){self.delData(k, m)}}(ind, ret.span);
	return ret.span;
}

sel_table.prototype.refreshRow = function (ret, ind, len) {
	if (this.delRow(ind, len)) this.insRow(ret, this.tbody.rows.length - ind - len);
	return;
}

sel_table.prototype.modData = function (k, len) {

	var trow = this.tbody.rows[this.tbody.rows.length - k - len];
	var req = {};
	var ret = {};

	req.keys = [];
	req.vals = [];
	var el = getElementsByClassName("primary","span", trow);
	for (var i=0; i<this.primary.length; i++) {
		req.keys[i] = this.primary[i];
		for (var j=0; j<el.length; j++) {
		if (el[j].name != this.primary[i]) continue;
			req.vals[i] = el[j].innerHTML;
			break;
		}
	}
	ret = this.getData(req);
	if (typeof ret == "boolean" || ret.status != 0) return false;

	if (wndPop) wndPop.close();
	var str = this.htm + "?mode=upd&table=" + this.type + "&ind=" + k + "&len=" + len + "&data=" + JSON.stringify(ret.data[0]);
	wndPop = window.open(str, "_blank", "menubar=no,scrollbars=yes,toolbar=no,location=no,directories=no,status=no,resizable=yes,width=" + this.w + ",height=" + this.h + ",left=200,top=100");
	if (wndPop){
		return true;
	} else {
		alert ("Couldn't open the new window.\nCheck the browser settings");
		return false;
	}
	return;
}

sel_table.prototype.delData = function (k, len) {
	var trow = this.tbody.rows[this.tbody.rows.length - k - len];
	var req = {};
	var ret = {};

	req.keys = [];
	req.vals = [];
	var el = getElementsByClassName("primary","span", trow);
	for (var i=0; i<this.primary.length; i++) {
		req.keys[i] = this.primary[i];
		for (var j=0; j<el.length; j++) {
		if (el[j].name != this.primary[i]) continue;
			req.vals[i] = el[j].innerHTML;
			break;
		}
	}
	ret = this.getData(req);
	if (typeof ret == "boolean" || ret.status != 0) return false;
	if (wndPop) wndPop.close();
	var str = this.htm + "?mode=del&table=" + this.type + "&ind=" + k + "&len=" + len + "&data=" + JSON.stringify(ret.data[0]);
	wndPop = window.open(str, "_blank", "menubar=no,scrollbars=yes,toolbar=no,location=no,directories=no,status=no,resizable=yes,width=" + this.w + ",height=" + this.h + ",left=200,top=100");
	if (wndPop){
		return true;
	} else {
		alert ("Couldn't open the new window.\nCheck the browser settings");
		return false;
	}
	return;
}
sel_table.prototype.insertData = function () {
	if (wndPop) wndPop.close();
	var str = this.htm + "?mode=ins&table=" + this.type;
	wndPop = window.open(str, "_blank", "menubar=no,scrollbars=yes,toolbar=no,location=no,directories=no,status=no,resizable=yes,width=" + this.w + ",height=" + this.h + ",left=200,top=100");
	if (wndPop){
		return true;
	} else {
		alert ("Couldn't open the new window.\nCheck the browser settings");
		return false;
	}
	return;
}

sel_table.prototype.refreshData = function () {
	var sel = this.getData({});
	if (!sel || sel.status != 0) return false;                                                                    
	this.drawTable(sel.data);
	return true;
}

sel_table.prototype.getData = function (req) {

	req.query = queries[this.type];
	if (req.keys) {
		req.bind = 1;
		var sql_qual = [];
		for (var i=0; i<req.keys.length; i++) {
			sql_qual.push(req.keys[i] + " = ?");			
		}
		req.query += " where " + sql_qual.join(" and ");
	} else {
		req.bind = 0;
	}
	req.query += " order by " + sorts[this.type];
	var ret = ui_req_s("cmdSelect", req);
	if (typeof ret == "number") {                                                            
		alert("CGI Error : " + ret);
		return false;                                                                    
	} else if (ret.status != 0) {
		alert("DB error : " + ret.retMsg + "\nReason : " + ret.retNative);
		return ret;
	}        
	var lst = [];
	var prev = {};
	if (typeof ret["data"] == "undefined") return ret;
	for (var i=0; i<ret["data"].length; i++) {
		procStruct(this.type, lst, 0, prev, ret["data"][i]);
		for (var p in ret["data"][i]) {
			prev[p] = ret["data"][i][p];
		}
	}

	ret["data"] = lst;
	return ret;
}


