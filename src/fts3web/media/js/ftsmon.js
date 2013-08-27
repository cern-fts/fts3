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
 		when('/archive', {templateUrl: STATIC_ROOT + 'html/jobs/index.html',
				          controller:  ArchiveCtrl,
				          resolve:     ArchiveCtrl.resolve}).

	    when('/queue/', {templateUrl: STATIC_ROOT + 'html/queue/queue.html',
	    	             controller:  JobQueueCtrl,
	    	             resolve:     JobQueueCtrl.resolve}).
        when('/queue/details', {templateUrl: STATIC_ROOT + 'html/queue/detailed.html',
					            controller:  JobQueueDetailedCtrl,
					            resolve:     JobQueueDetailedCtrl.resolve}).

		when('/staging/', {templateUrl: STATIC_ROOT + 'html/staging.html',
			               controller:  StagingCtrl,
					       resolve:     StagingCtrl.resolve}).
					       
        when('/optimizer/', {templateUrl: STATIC_ROOT + 'html/optimizer/optimizer.html',
				             controller:  OptimizerCtrl,
						     resolve:     OptimizerCtrl.resolve}).
        when('/optimizer/detailed', {templateUrl: STATIC_ROOT + 'html/optimizer/detailed.html',
					                 controller:  OptimizerDetailedCtrl,
							         resolve:     OptimizerDetailedCtrl.resolve}).

        when('/errors/', {templateUrl: STATIC_ROOT + 'html/errors/errors.html',
                          controller:  ErrorsCtrl,
	                      resolve:     ErrorsCtrl.resolve}).
        when('/errors/list', {templateUrl: STATIC_ROOT + 'html/errors/list.html',
	                          controller:  FilesWithErrorCtrl,
		                      resolve:     FilesWithErrorCtrl.resolve}).
		                      
        when('/configaudit', {templateUrl: STATIC_ROOT + 'html/config.html',
		                      controller:  ConfigCtrl,
			                  resolve:     ConfigCtrl.resolve}).
							         
		otherwise({templateUrl: STATIC_ROOT + 'html/404.html'});
})
.filter('escape', function() {
	return window.escape;
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
