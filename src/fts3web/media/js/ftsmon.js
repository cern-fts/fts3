angular.module('ftsmon', ['ftsmon.resources', 'ui.bootstrap']).
config(function($routeProvider) {
	$routeProvider.
		when('/', {templateUrl: STATIC_ROOT + 'html/jobs/index.html',
				   controller:  JobListCtrl,
			       resolve:     JobListCtrl.resolve}).

		when('/job/:jobId', {templateUrl: STATIC_ROOT + 'html/jobs/view.html',
			                 controller:  JobViewCtrl,
			                 resolve:     JobViewCtrl.resolve,
			                 reloadOnSearch: false}).

	    when('/queue/', {templateUrl: STATIC_ROOT + 'html/jobs/queue.html',
	    	             controller:  JobQueueCtrl,
	    	             resolve:     JobQueueCtrl.resolve,
	    	             reloadOnSearch: true}).

		otherwise({templateUrl: STATIC_ROOT + 'html/404.html'});
});

/** Show loading **/

function loading($rootScope)
{
	$rootScope.loading = true;	
}

function stopLoading($rootScope)
{
	$rootScope.loading = false;
}

/** Join states with commas */
function joinStates(states)
{
	var str = '';
	if (states.submitted)
		str += 'SUBMITTED,';
	if (states.ready)
		str += 'READY,';
	if (states.active)
		str += 'ACTIVE,';
	if (states.canceled)
		str += 'CANCELED,';
	if (states.failed)
		str += 'FAILED,';
	if (states.finished)
		str += 'FINISHED,';
	if (states.finisheddirty)
		str += 'FINISHEDDIRTY,';
	if (str.length > 0)
		str = str.slice(0, -1);
	return str;
}

function statesFromString(str)
{
	var st = {ready: false, active: false, canceled: false, failed: false, finished: false, finisheddirty: false};
	
	if (typeof(str) != "undefined") {
		var states = str.split(',');
		
		for (var i in states) {
			if (states[i] == 'SUBMITTED')
				st.submitted= true;
			if (states[i] == 'READY')
				st.ready = true;
			if (states[i] == 'ACTIVE')
				st.active = true;
			if (states[i] == 'CANCELED')
				st.canceled = true;
			if (states[i] == 'FAILED')
				st.failed = true;
			if (states[i] == 'FINISHED')
				st.finished = true;
			if (states[i] == 'FINISHEDDIRTY')
				st.finisheddirty = true;
		}
	}
	
	return st;
}

/** Get default if undefined **/
function undefinedAsEmpty(v)
{
	if (typeof(v) != 'undefined')
		return v;
	else
		return '';
}

function validVo(v)
{
	if (typeof(v) == 'string')
		return v;
	else
		return '';
}
