
// Overview
function _generateOverviewPlots(stats)
{
    var queueColors = ["#fae932", "#9ed5ff", "#006dcc", "#990099"];
    var lastHourColors = ["#5bb75b", "#bd362f"];
    return {
        queue: {
            data: [
                {x: "submitted", y: [stats.lasthour.submitted]},
                {x: "ready", y: [stats.lasthour.ready]},
                {x: "active", y: [stats.lasthour.active]},
                {x: "staging", y: [stats.lasthour.staging]},
            ],
            config: {
                title: 'Queue',
                legend: {position: 'right', display: true},
                innerRadius: 50,
                colors: queueColors,
                labels: true
            }
        },
        lasthour: {
            data: [
                {x: "finished", y: [stats.lasthour.finished]},
                {x: "failed", y: [stats.lasthour.failed]},
            ],
            config: {
                title: 'Last hour',
                legend: {position: 'right', display: true},
                innerRadius: 50,
                colors: lastHourColors,
                labels: true
            }
        }
    };
}

function StatsOverviewCtrl($rootScope, $routeParams, $location, $scope, stats, Statistics, Unique)
{
    $scope.stats = stats;
    $scope.host = $location.$$search.hostname;

    $scope.hostnames = Unique.query({field: 'hostnames'});

    $scope.filterHost = function(host) {
        var filter = $location.$$search;
        if (host)
            filter.hostname = host;
        else
            delete filter.hostname;
        $location.search(filter);
        $scope.host = host;
    }

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        var filter = $location.$$search;
        loading($rootScope);
        Statistics.query(filter,
            function(updatedStats) {
                $scope.stats = updatedStats;
                $scope.plots = _generateOverviewPlots($scope.stats);
                stopLoading($rootScope);
            },
            genericFailureMethod(null, $rootScope, $location)
        );
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    $scope.plots = _generateOverviewPlots($scope.stats);
}


StatsOverviewCtrl.resolve = {
    stats: function($route, $rootScope, $location, $q, Statistics) {
        loading($rootScope);

        var deferred = $q.defer();

        Statistics.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

// Per server

function _dataByState(servers, state)
{
    var points = [];
    var total = 0;
    for (server in servers) {
        if (server[0] != '$') {
            var value = undefinedAsZero(servers[server][state]);
            total += value;
            points.push({x: server, y: [value]});
        }
    }
    if (points)
        return points;
    else
        return null;
}

function _generateServerPlots(servers)
{
    var serverColors = [
        '#366DD8', '#D836BE', '#D8A136', '#36D850', '#5036D8', '#D8366D', '#BED836', '#36D8A1', '#A136D8', '#D85036'
    ];
    return {
        submit: {
            data: _dataByState(servers, 'submissions'),
            config: {
                title: 'Submissions',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: serverColors,
                labels: true
            }
        },
        executed: {
            data: _dataByState(servers, 'transfers'),
            config: {
                title: 'Executed',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: serverColors,
                labels: true
            }
        },
        active: {
            data: _dataByState(servers, 'active'),
            config: {
                title: 'Active',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: serverColors,
                labels: true
            }
        },
        staging: {
            data: _dataByState(servers, 'staging'),
            config: {
                title: 'Staging',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: serverColors,
                labels: true
            }
        },
        started: {
            data: _dataByState(servers, 'started'),
            config: {
                title: 'Staging started',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: serverColors,
                labels: true
            }
        }
    };
}

function StatsServersCtrl($rootScope, $location, $scope, servers, Servers)
{
    $scope.servers = servers;

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        var filter = $location.$$search;
        loading($rootScope);
        Servers.query(filter, function (updatedServers) {
            for(var server in updatedServers) {
                if (server.toString().substring(0, 1) != '$')
                    updatedServers[server].show = $scope.servers[server].show;
            }
            $scope.servers = updatedServers;
            $scope.plots = _generateServerPlots($scope.servers);
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    $scope.plots = _generateServerPlots($scope.servers);
}


StatsServersCtrl.resolve = {
    servers: function($rootScope, $location, $q, Servers) {
        loading($rootScope);

        var deferred = $q.defer();

        Servers.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

// Database
function StatsDatabaseCtrl($rootScope, $location, $scope, database, Database)
{
    $scope.database = database;

    $scope.worryColor = function(worryValue) {
        var h = 0.33 * (1 - worryValue);
        var rgb = hsvToRgb(h, 1, 0.8);
        var color = 'rgba(' + Math.ceil(rgb[0]) + ',' + Math.ceil(rgb[1]) + ',' + Math.ceil(rgb[2]) + ', 0.8)'
        return {'border-left': 'solid 10px ' + color};
    }

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        var filter = $location.$$search;
        loading($rootScope);
        Database.query(filter,
            function(updatedStats) {
                $scope.database = updatedStats;
                stopLoading($rootScope);
            },
            genericFailureMethod(null, $rootScope, $location)
        );
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });
}


StatsDatabaseCtrl.resolve = {
    database: function ($rootScope, $location, $q, Database) {
        loading($rootScope);

        var deferred = $q.defer();

        Database.query($location.$$search,
            genericSuccessMethod(deferred, $rootScope),
            genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}


// Per VO
function _generatePerVoPlots(vos)
{
    var perVoColors = [
        '#366DD8', '#D836BE', '#D8A136', '#36D850', '#5036D8', '#D8366D', '#BED836', '#36D8A1', '#A136D8', '#D85036'
    ];

    return {
        active: {
            data: _dataByState(vos, 'active'),
            config: {
                title: 'Active',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: perVoColors,
                labels: true
            }
        },
        submitted: {
            data: _dataByState(vos, 'submitted'),
            config: {
                title: 'Submitted',
                legend: {position: 'left', display: true},
                innerRadius: 50,
                colors: perVoColors,
                labels: true
            }
        }
    };
}

function StatsVosCtrl($rootScope, $location, $scope, vos, StatsVO, Unique)
{
    $scope.vos = vos;

    // Filter
    $scope.unique = {
        sources: Unique('sources'),
        destinations: Unique('destinations')
    }

    $scope.filter = {
        'source_se': validString($location.$$search.source_se),
        'dest_se':   validString($location.$$search.dest_se),
    }

    $scope.applyFilters = function() {
        $location.search($scope.filter);
    }

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        var filter = $location.$$search;
        loading($rootScope);
        StatsVO.query(filter,
        function(updatedVos) {
            $scope.vos = updatedVos;
            $scope.plots = _generatePerVoPlots($scope.vos);
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    $scope.plots = _generatePerVoPlots($scope.vos);
}


StatsVosCtrl.resolve = {
    vos: function($rootScope, $location, $q, StatsVO) {
        loading($rootScope);

        var deferred = $q.defer();

        StatsVO.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

// Transfer volume
function TransferVolumeCtrl($location, $scope, volumes)
{
    $scope.volumes = volumes;

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };
}

TransferVolumeCtrl.resolve = {
    volumes: function($rootScope, $location, $q, TransferVolume) {
        loading($rootScope);

        var deferred = $q.defer();

        TransferVolume.query($location.$$search,
                genericSuccessMethod(deferred, $rootScope),
                genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

// Profiling

function StatsProfilingCtrl($rootScope, $location, $scope, profile, Profile)
{
    $scope.profile = profile;

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        loading($rootScope);
        var filter = $location.$$search;
        Profile.query(filter, function(updatedProfile) {
            $scope.profile = updatedProfile;
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });
}


StatsProfilingCtrl.resolve = {
    profile: function($rootScope, $location, $q, Profile) {
        loading($rootScope);

        var deferred = $q.defer();

        Profile.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}


function SlowQueriesCtrl($location, $scope, slowQueries)
{
    $scope.slowQueries = slowQueries;
}

SlowQueriesCtrl.resolve = {
    slowQueries: function($rootScope, $location, $q, SlowQueries) {
        loading($rootScope);

        var deferred = $q.defer();

        SlowQueries.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

// TURLS
function TurlsCtrl($location, $scope, turls)
{
    $scope.turls = turls;

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

    $scope.filterBy = function(filter) {
        $location.search(filter);
    }
}

TurlsCtrl.resolve = {
    turls: function($rootScope, $location, $q, Turls) {
        loading($rootScope);

        var deferred = $q.defer();

        Turls.query($location.$$search,
            genericSuccessMethod(deferred, $rootScope),
            genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}
