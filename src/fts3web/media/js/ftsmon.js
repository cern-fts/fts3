angular.module('ftsmon', ['ftsmon.resources', 'ui.bootstrap']).
config(function($routeProvider) {
	$routeProvider.
		when('/',                     {templateUrl: STATIC_ROOT + 'html/overview.html',
				                       controller:  OverviewCtrl,
			                           resolve:     OverviewCtrl.resolve}).
        when('/jobs',                 {templateUrl: STATIC_ROOT + 'html/jobs/index.html',
					                       controller:  JobListCtrl,
				                           resolve:     JobListCtrl.resolve}).
		when('/job/:jobId',           {templateUrl: STATIC_ROOT + 'html/jobs/view.html',
			                           controller:  JobViewCtrl,
			                           resolve:     JobViewCtrl.resolve}).
 		when('/archive',              {templateUrl: STATIC_ROOT + 'html/jobs/index.html',
				                       controller:  ArchiveCtrl,
				                       resolve:     ArchiveCtrl.resolve}).

        when('/transfers',            {templateUrl: STATIC_ROOT + 'html/transfers.html',
					                   controller:  TransfersCtrl,
					                   resolve:     TransfersCtrl.resolve}).

		when('/staging/',             {templateUrl: STATIC_ROOT + 'html/staging.html',
			                           controller:  StagingCtrl,
					                   resolve:     StagingCtrl.resolve}).
					       
        when('/optimizer/',           {templateUrl: STATIC_ROOT + 'html/optimizer/optimizer.html',
				                       controller:  OptimizerCtrl,
						               resolve:     OptimizerCtrl.resolve}).
        when('/optimizer/detailed',   {templateUrl: STATIC_ROOT + 'html/optimizer/detailed.html',
					                   controller:  OptimizerDetailedCtrl,
							           resolve:     OptimizerDetailedCtrl.resolve}).
        when('/optimizer/decisions',  {templateUrl: STATIC_ROOT + 'html/optimizer/decisions.html',
			                           controller:  OptimizerDecisionsCtrl,
							           resolve:     OptimizerDecisionsCtrl.resolve}).
							           
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
        when('/statistics/vos',       {templateUrl: STATIC_ROOT + 'html/statistics/vos.html',
                                       controller:  StatsVosCtrl,
				                       resolve:     StatsVosCtrl.resolve}).
        when('/statistics/profiling', {templateUrl: STATIC_ROOT + 'html/statistics/profiling.html',
                                       controller:  StatsProfilingCtrl,
                                       resolve:     StatsProfilingCtrl.resolve}).
							         
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
			// If only value is specified, then the list is a dictionary,
			// value specify the field to plot for each entry
			else if (attr.value)
				plotDictionary(scope, list, attr.value);
			// Otherwise, we don't know!
			else
				throw new Error('Invalid usage of plot!');

			// Set title and colors
			scope.title  = attr.title;
			if (attr.colors)
				scope.colors = attr.colors; 
		}
	};
})
.directive('log', function() {
	return {
		restrict: 'A',
		scope: 'isolate',
		replace: true,
		template: '<a href="{{logUrl}}">{{log}}</a>',
		link: function(scope, element, attr) {
			scope.log = scope.$eval(attr.log);
			scope.logUrl = LOG_BASE_URL.replace('%(host)', scope.$eval(attr.host)) + scope.log;
		}
	}
})
.directive('optionalNumber', function() {
	return {
		restrict: 'A',
		scope: 'isolate',
		replace: false,
		template: '{{ value }} {{ suffix }}',
		link: function(scope, element, attr) {
			scope.value = scope.$eval(attr.optionalNumber);
			scope.decimals = scope.$eval(attr.decimals);
			if (scope.value == null || scope.value == undefined) {
				scope.value = '-';
			}
			else {
				scope.value = Number(scope.value).toFixed(scope.decimals);
				scope.suffix = attr.suffix;
			}
		}
	}
})
.directive('orderBy', ['$location', function($location) {
	return {
		restrict: 'A',
		scope: 'isolate',
		replace: true,
		link: function(scope, element, attr) {
			var content = element.text();
			var field = attr.orderBy;
			var orderedBy = $location.search().orderby;
			if (!orderedBy)
				orderedBy = '';
			
			// Differentiate between field and ordering (asc/desc)
			var orderedDesc = false;
			if (orderedBy && orderedBy[0] == '-') {
				orderedDesc = true;
				orderedBy = orderedBy.slice(1);
			}
			
			// Now, add the icon if this is the field used for the ordering
			var icon = '';
			if (orderedBy == field) {
				if (orderedDesc)
					icon = '<i class="icon-arrow-down"></i>';
				else
					icon = '<i class="icon-arrow-up"></i>';
			}
			
			// HTML
			var html = '<span class="orderby">' + icon + content + '</span>';
			var replacement = angular.element(html);
			
			// Bind the method
			replacement.bind('click', function() {
				if (orderedDesc && field == orderedBy)
					$location.search('orderby', field);
				else
					$location.search('orderby', '-' + field);
				scope.$apply();
			});
			
			// Replace
			element.replaceWith(replacement);
		}
	}
}])
.run(function($rootScope, $location) {
	$rootScope.searchJob = function() {
		$location.path('/job/' + $rootScope.jobId);
	}
})
;

/** Refresh interval in ms */
var REFRESH_INTERVAL = 60000;

/** Pie plotting */
function plotArrayOfObjects(scope, list, labelAttr, valueAttr)
{
	var labelStr = '', valueStr = '';
	for (var i in list) {
		var item = list[i];
		labelStr += firstUpper(item[labelAttr]) + ',';
		valueStr += undefinedAsZero(item[valueAttr]) + ',';
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
		valueStr += undefinedAsZero(obj[label]) + ',';
	}
	
	scope.labels = labelStr;
	scope.values = valueStr;
}

function plotDictionary(scope, list, value)
{
	var labelStr = '', valueStr = '';
	for (var i  in list) {
		if (i[0] != '$') {
			labelStr += i + ',';
			valueStr += undefinedAsZero(list[i][value]) + ',';
		}
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
	if (states.not_used)
		str += 'NOT_USED,';
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
			if (states[i] == 'NOT_USED')
				st.not_used = true;
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

function undefinedAsZero(v)
{
	if (typeof(v) != 'undefined')
		return v;
	else
		return 0;
}

function validVo(v)
{
	if (typeof(v) == 'string')
		return v;
	else
		return '';
}
