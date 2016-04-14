// Config audit
function ConfigAuditCtrl($rootScope, $location, $scope, config, ConfigAudit)
{
    $scope.config = config;

    // Filter
    $scope.filter = {
        action:   validString($location.search().action),
        user:     validString($location.search().user),
        contains: validString($location.search().contains)
    };

    $scope.applyFilters = function() {
        $location.search({
            page:        1,
            action:   $scope.filter.action,
            user:     $scope.filter.user,
            contains: $scope.filter.contains
        });
    }

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        loading($rootScope);
        var filter = $location.search();
        filter.page = $scope.config.page;
        ConfigAudit.query(filter, function(updatedConfigAudit) {
            $scope.config = updatedConfigAudit;
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    $scope.showFilterDialog = function() {
        document.getElementById('filterDialog').style.display = 'block';
    }
}

ConfigAuditCtrl.resolve = {
    config: function ($rootScope, $location, $route, $q, ConfigAudit) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigAudit.query($location.search(),
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

/// Config status
function ConfigStatusCtrl($location, $scope, server, debug)
{
    $scope.server = server;
    $scope.debug = debug;

    // On page change, reload
    $scope.debugPageChanged = function(newPage) {
        $location.search('debug_page', newPage);
    };
}

ConfigStatusCtrl.resolve = {
    server: function ($rootScope, $location, $route, $q, ConfigServer) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigServer.all(
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    },

    debug: function($rootScope, $location, $route, $q, ConfigDebug) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigDebug.query({"page": $location.search().debug_page},
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

/// Config links
function ConfigLinksCtrl($location, $scope, links) {
    $scope.links = links;

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };
}

ConfigLinksCtrl.resolve = {
    links: function($rootScope, $location, $route, $q, ConfigLinks) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigLinks.query($location.search(),
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    },
}

/// Config limits
function ConfigLimitsCtrl($location, $scope, limits) {
    $scope.limits = limits;

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };
}

ConfigLimitsCtrl.resolve = {
    limits: function($rootScope, $location, $route, $q, ConfigLimits) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigLimits.query($location.search(),
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

function ConfigFixedCtrl($location, $scope, fixed) {
    $scope.fixed = fixed;

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };
}

ConfigFixedCtrl.resolve = {
    fixed: function($rootScope, $location, $route, $q, ConfigFixed) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigFixed.query($location.search(),
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

/// Gfal2 configuration
function Gfal2Ctrl($location, $scope, gfal2) {
    $scope.gfal2 = gfal2;
}

Gfal2Ctrl.resolve = {
    gfal2: function($rootScope, $location, $q, ConfigGfal2) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigGfal2.query($location.search(),
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

/// Activities
function ActivitiesCtrl($location, $scope, activities) {
    $scope.activities = activities;
    $scope.filter = $location.search();
}

ActivitiesCtrl.resolve = {
    activities: function($rootScope, $location, $q, ConfigActivities) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigActivities.query($location.search(),
            genericSuccessMethod(deferred, $rootScope),
            genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}


/// Active per vo activities
function _plotDataFromCount(activeCount, key)
{
    var points = [];
    var total = 0;
    for (var activity in activeCount) {
        if (activity[0] != '$') {
            var value = undefinedAsZero(activeCount[activity]['count'][key]);
            total += value;
            points.push({x: activity, y: [value]});
        }
    }
    if (points)
        return points;
    else
        return null;
}

function VoActivePerActivitiesCtrl($location, $scope, $route, activeCount) {
    $scope.vo = $route.current.params.vo;
    $scope.activeCount = activeCount;

    var colors = [
        '#366DD8', '#D836BE', '#D8A136', '#36D850', '#5036D8', '#D8366D', '#BED836', '#36D8A1', '#A136D8', '#D85036'
    ];
    $scope.plots = {
        submittedCount: {
            data: _plotDataFromCount(activeCount, 'SUBMITTED'),
            config: {
                title: 'Submitted per activity',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: colors,
                labels: true
            }
        },
        activeCount: {
            data: _plotDataFromCount(activeCount, 'ACTIVE'),
            config: {
                title: 'Actives per activity',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: colors,
                labels: true
            }
        }
    }
}

VoActivePerActivitiesCtrl.resolve = {
    activeCount: function($rootScope, $location, $route, $q, ActivePerActivity) {
        loading($rootScope);

        var deferred = $q.defer();

        var filter = $location.$$search;
        filter.vo = $route.current.params.vo;

        ActivePerActivity.query(filter,
            genericSuccessMethod(deferred, $rootScope),
            genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}
