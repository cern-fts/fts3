angular.module('ftsmon', ['ftsmon.resources', 'ftsmon.plots', 'ftsmon.global_filter', 'ui.bootstrap']).
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

        when('/transfers',            {templateUrl: STATIC_ROOT + 'html/transfers.html',
                                       controller:  TransfersCtrl,
                                       resolve:     TransfersCtrl.resolve}).

        when('/optimizer/',           {templateUrl: STATIC_ROOT + 'html/optimizer/optimizer.html',
                                       controller:  OptimizerCtrl,
                                       resolve:     OptimizerCtrl.resolve}).
        when('/optimizer/detailed',   {templateUrl: STATIC_ROOT + 'html/optimizer/detailed.html',
                                       controller:  OptimizerDetailedCtrl,
                                       resolve:     OptimizerDetailedCtrl.resolve}).

        when('/errors/',              {redirectTo:  ErrorsCtrl.redirectTo}).
        when('/errors/pairs',         {templateUrl: STATIC_ROOT + 'html/errors/pairs.html',
                                       controller:  ErrorsCtrl,
                                       resolve:     ErrorsCtrl.resolve}).
        when('/errors/list',          {templateUrl: STATIC_ROOT + 'html/errors/list.html',
                                       controller:  ErrorsForPairCtrl,
                                       resolve:     ErrorsForPairCtrl.resolve}).

        when('/config/audit',         {templateUrl: STATIC_ROOT + 'html/config/audit.html',
                                       controller:  ConfigAuditCtrl,
                                       resolve:     ConfigAuditCtrl.resolve}).
        when('/config/status',        {templateUrl: STATIC_ROOT + 'html/config/status.html',
                                       controller:  ConfigStatusCtrl,
                                       resolve:     ConfigStatusCtrl.resolve}).
        when('/config/links',         {templateUrl: STATIC_ROOT + 'html/config/links.html',
                                       controller:  ConfigLinksCtrl,
                                       resolve:     ConfigLinksCtrl.resolve}).
        when('/config/limits',        {templateUrl: STATIC_ROOT + 'html/config/limits.html',
                                       controller:  ConfigLimitsCtrl,
                                       resolve:     ConfigLimitsCtrl.resolve}).
        when('/config/gfal2',         {templateUrl: STATIC_ROOT + 'html/config/gfal2.html',
                                       controller:  Gfal2Ctrl,
                                       resolve:     Gfal2Ctrl.resolve}).

        when('/statistics/overview',  {templateUrl: STATIC_ROOT + 'html/statistics/overview.html',
                                       controller:  StatsOverviewCtrl,
                                       resolve:     StatsOverviewCtrl.resolve}).
        when('/statistics/servers',   {templateUrl: STATIC_ROOT + 'html/statistics/servers.html',
                                       controller:  StatsServersCtrl,
                                       resolve:     StatsServersCtrl.resolve}).
        when('/statistics/vos',       {templateUrl: STATIC_ROOT + 'html/statistics/vos.html',
                                       controller:  StatsVosCtrl,
                                       resolve:     StatsVosCtrl.resolve}).
       when('/statistics/volume',     {templateUrl: STATIC_ROOT + 'html/statistics/volume.html',
                                       controller:  TransferVolumeCtrl,
                                       resolve:     TransferVolumeCtrl.resolve}).
       when('/statistics/turls',      {templateUrl: STATIC_ROOT + 'html/statistics/turls.html',
                                       controller:  TurlsCtrl,
                                       resolve:     TurlsCtrl.resolve}).

        when('/statistics/profiling', {templateUrl: STATIC_ROOT + 'html/statistics/profiling.html',
                                       controller:  StatsProfilingCtrl,
                                       resolve:     StatsProfilingCtrl.resolve}).
        when('/statistics/slowqueries', {templateUrl: STATIC_ROOT + 'html/statistics/slowqueries.html',
                                         controller:  SlowQueriesCtrl,
                                         resolve:     SlowQueriesCtrl.resolve}).

        when('/500',                    {templateUrl: STATIC_ROOT + 'html/500.html'}).

        otherwise({templateUrl: STATIC_ROOT + 'html/404.html'});
})
.filter('escape', function() {
    return window.escape;
})
.filter('hex', function() {
	return function(value) {
		hex = value.toString(16);
		return ("0000".substr(0, 4 - hex.length) + hex).toUpperCase();
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
            scope.decimals = scope.$eval(attr.decimals);
            scope.$watch(attr.optionalNumber, function(val) {
                scope.value = val;
                if (scope.value == null || scope.value == undefined) {
                    scope.value = '-';
                }
                else {
                    scope.value = Number(scope.value).toFixed(scope.decimals);
                    scope.suffix = attr.suffix;
                }
            });
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
            var title = attr.title;
            var orderedBy = $location.search().orderby;
            if (!orderedBy)
                orderedBy = '';

            if (!title)
                title = 'Order by ' + content;

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
            var html = '<span class="orderby" title="' + title + '">' + icon + content + '</span>';
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
        $rootScope.clearGlobalFilters();
        $location.path('/job/' + $rootScope.jobId).search({});
    }
})
.filter('safeFilter', function($filter) {
    return function(list, expr) {
        if (typeof(list) == 'undefined')
            return [];
        return $filter('filter')(list, expr);
    }
})
.filter('filesize', function($filter) {
    return function(bytes) {
    	if (bytes == null)
    		return '-';
    	else if (bytes < 1024)
            return bytes.toString() + ' bytes';
        else if (bytes < 1048576)
            return (bytes / 1024.0).toFixed(2).toString() + ' KiB';
        else if (bytes < 1073741824)
            return (bytes / 1048576.0).toFixed(2).toString() + ' MiB';
        else
            return (bytes / 1073741824.0).toFixed(2).toString() + ' GiB';
    }
})
;

/** Refresh interval in ms */
var REFRESH_INTERVAL = 60000;

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
    if (states.staging)
        str += 'STAGING,';
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
    if (states.started)
        str += 'STARTED,';
    if (states.delete)
        str += 'DELETE,';
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
            if (states[i] == 'STAGING')
                st.staging = true;
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
            if (states[i] == 'STARTED')
                st.started = true;
            if (states[i] == 'DELETE')
                st.delete = true;
        }
    }

    return st;
}

/** Get default if undefined **/
function validString(v)
{
    if (typeof(v) == 'string')
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

function withDefault(v, def)
{
    if (typeof(v) != 'undefined')
        return v;
    else
        return def;
}

/** Generic callbacks */
function genericSuccessMethod(deferred, $rootScope) {
    return function(data) {
        deferred.resolve(data);
        stopLoading($rootScope);
    }
}

function genericFailureMethod(deferred, $rootScope, $location) {
    return function (response) {
        deferred.resolve(false);
        stopLoading($rootScope);
        if (response.status == 404)
            $location.path('/404');
        else if (response.status == 500)
            $location.path('/500');
        else
            $location.path('/generic-error');
    }
}
