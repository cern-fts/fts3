angular.module('ftsmon', ['ftsmon.resources', 'ui.bootstrap']).
config(function($routeProvider) {
	$routeProvider.
		when('/',                     {templateUrl: STATIC_ROOT + 'html/jobs/index.html',
				                       controller:  JobListCtrl,
			                           resolve:     JobListCtrl.resolve}).
		when('/job/:jobId',           {templateUrl: STATIC_ROOT + 'html/jobs/view.html',
			                           controller:  JobViewCtrl,
			                           resolve:     JobViewCtrl.resolve}).
 		when('/archive',              {templateUrl: STATIC_ROOT + 'html/jobs/index.html',
				                       controller:  ArchiveCtrl,
				                       resolve:     ArchiveCtrl.resolve}).

	    when('/queue/',               {templateUrl: STATIC_ROOT + 'html/queue/queue.html',
	    	                           controller:  JobQueueCtrl,
	    	                           resolve:     JobQueueCtrl.resolve}).
        when('/queue/details',        {templateUrl: STATIC_ROOT + 'html/queue/detailed.html',
					                   controller:  JobQueueDetailedCtrl,
					                   resolve:     JobQueueDetailedCtrl.resolve}).

		when('/staging/',             {templateUrl: STATIC_ROOT + 'html/staging.html',
			                           controller:  StagingCtrl,
					                   resolve:     StagingCtrl.resolve}).
					       
        when('/optimizer/',           {templateUrl: STATIC_ROOT + 'html/optimizer/optimizer.html',
				                       controller:  OptimizerCtrl,
						               resolve:     OptimizerCtrl.resolve}).
        when('/optimizer/detailed',   {templateUrl: STATIC_ROOT + 'html/optimizer/detailed.html',
					                   controller:  OptimizerDetailedCtrl,
							           resolve:     OptimizerDetailedCtrl.resolve}).

        when('/errors/',              {templateUrl: STATIC_ROOT + 'html/errors/errors.html',
                                       controller:  ErrorsCtrl,
	                                   resolve:     ErrorsCtrl.resolve}).
        when('/errors/list',          {templateUrl: STATIC_ROOT + 'html/errors/list.html',
	                                   controller:  FilesWithErrorCtrl,
		                               resolve:     FilesWithErrorCtrl.resolve}).
		                      
        when('/configaudit',          {templateUrl: STATIC_ROOT + 'html/config.html',
		                               controller:  ConfigCtrl,
			                           resolve:     ConfigCtrl.resolve}).
			                  
        when('/statistics/overview',  {templateUrl: STATIC_ROOT + 'html/statistics/overview.html',
			                           controller:  StatsOverviewCtrl,
				                       resolve:     StatsOverviewCtrl.resolve}).
        when('/statistics/servers',   {templateUrl: STATIC_ROOT + 'html/statistics/servers.html',
			                           controller:  StatsServersCtrl,
				                       resolve:     StatsServersCtrl.resolve}).
        when('/statistics/pairs',     {templateUrl: STATIC_ROOT + 'html/statistics/pairs.html',
		                               controller:  StatsPairsCtrl,
				                       resolve:     StatsPairsCtrl.resolve}).
							         
		otherwise({templateUrl: STATIC_ROOT + 'html/404.html'});
})
.filter('escape', function() {
	return window.escape;
})
.directive('plot', function() {
	return {
		restrict: 'E',
		scope: 'isolate',
		template: '<img ng-src="plot/pie?t={{title}}&l={{labels}}&v={{values}}&c={{colors}}"></img>',
		link: function(scope, iterStartElement, attr) {
			var list = scope.$eval(attr.list);
			
			// If label and value are specified, list is an array of
			// objects, and label and value specify the fields to use
			if (attr.label && attr.value)
				plotArrayOfObjects(scope, list, attr.label, attr.value);
			// If labels is specified, then list is an object, and labels
			// specify the attributes to plot
			else if (attr.labels)
				plotObject(scope, list, attr.labels.split(','));
			// Otherwise, we don't know!
			else
				throw new Error('Invalid usage of plot!');

			// Set title and colors
			scope.title  = attr.title;
			if (attr.colors)
				scope.colors = attr.colors; 
		}
	};
});

/** Pie plotting */
function plotArrayOfObjects(scope, list, labelAttr, valueAttr)
{
	var labelStr = '', valueStr = '';
	for (var i in list) {
		var item = list[i];
		labelStr += firstUpper(item[labelAttr]) + ',';
		valueStr += item[valueAttr] + ',';
	}
	
	scope.labels = labelStr;
	scope.values = valueStr;
}

function plotObject(scope, obj, labelsAttr)
{
	var labelStr = '', valueStr = '';
	for (var i in labelsAttr) {
		var label = labelsAttr[i];
		labelStr += firstUpper(label) + ',';
		valueStr += obj[label] + ',';
	}
	
	scope.labels = labelStr;
	scope.values = valueStr;
}

/** First letter uppercase */
function firstUpper(str)
{
	return str.charAt(0).toUpperCase() + str.substr(1).toLowerCase();
}

/** Show loading **/
var nLoading = 0;
function loading($rootScope)
{
	nLoading += 1;
	if (nLoading)
		$rootScope.loading = true;	
}

function stopLoading($rootScope)
{
	if (nLoading > 0)
		nLoading -= 1;
	if (!nLoading)
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
