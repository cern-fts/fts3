var _ui_xmlhttp_factories = [
        function() {return new XMLHttpRequest()},
        function() {return new ActiveXObject("Msxml2.XMLHTTP")},
        function() {return new ActiveXObject("Msxml3.XMLHTTP")},
        function() {return new ActiveXObject("Microsoft.XMLHTTP")}
];
function _ui_xmlhttp() {
        var xmlhttp = false;
        for(var i in _ui_xmlhttp_factories) {
                try {
                        xmlhttp = _ui_xmlhttp_factories[i]();
                } catch(e) {
                        continue;
                }
		break;
        }
        return xmlhttp;
}
function _ui_create_pkt(cmd, arg) {                                                                                                             
        var pkt = cmd + "\n";
        try {
			pkt += JSON.stringify(arg);
        } catch(e) {
                return false;
        }
        return pkt;
}                                                                                                                                                    
function _ui_parse_pkt(pkt) {
        var ret = new Array();
        try {
			ret = JSON.parse(pkt);
        } catch(e) {
                return false;
        }
        return ret;
}
function _ui_cb(req, cb, uid) {
        if(req.readyState != 4) return true;
        if(req.status != 200) {
                cb(req.status, uid);
        } else {
                cb(_ui_parse_pkt(req.responseText), uid);
        }
        return true;
}
function ui_req_s(cmd, arg) {
        var req = _ui_xmlhttp();
        if(! req) return false;
        var pkt = _ui_create_pkt(cmd, arg);
        req.open("post", "../cgi-bin/fts/fts3.cgi", false);
        req.setRequestHeader("Content-type", "text/plain; charset=utf-8");
        req.setRequestHeader("Content-length", pkt.length);
        req.setRequestHeader("Connection", "close");
        req.send(pkt);
        if(req.status != 200) {
                return req.status;
        } else {
                return _ui_parse_pkt(req.responseText);
        }
}
function ui_req_a(cb, cmd, arg) {                                                                                                               
        var req = _ui_xmlhttp();
        if(! req) return false;
        var now = new Date();
        var uid = now.getTime() + "" + Math.random();
        var pkt = _ui_create_pkt(cmd, arg);                                                                                                     
        req.open("post", "../cgi-bin/fts/fts3.cgi", true);
        req.setRequestHeader("Content-type", "text/plain; charset=utf-8");
        req.setRequestHeader("Content-length", pkt.length);
        req.setRequestHeader("Connection", "close");
        req.onreadystatechange = function() {return _ui_cb(req, cb, uid)};
        req.send(pkt);
        return uid;
}           
