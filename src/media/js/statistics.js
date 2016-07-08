
// Overview
function _generateOverviewPlots(stats)
{
    var queueColors = ["#fae932", "#9ed5ff", "#006dcc", "#990099"];
    var lastHourColors = ["#5bb75b", "#bd362f"];

    new Chart(document.getElementById("queuePlot"), {
        type: "doughnut",
        data: {
            labels: ['Submitted', 'Ready', 'Active', 'Staging'],
            datasets: [{
                data: [stats.lasthour.submitted, stats.lasthour.ready, stats.lasthour.active, stats.lasthour.staging],
                backgroundColor: queueColors
            }],
        },
    });

    new Chart(document.getElementById("lastHourPlot"), {
        type: "doughnut",
        data: {
            labels: ['Finished', 'Failed'],
            datasets: [{
                data: [stats.lasthour.finished, stats.lasthour.failed],
                backgroundColor: lastHourColors,
            }]
        }
    })
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
                _generateOverviewPlots($scope.stats);
                stopLoading($rootScope);
            },
            genericFailureMethod(null, $rootScope, $location)
        );
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    _generateOverviewPlots(stats);
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
            points.push(value)
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

    var labels = []
    for (server in servers) {
        if (server[0] != '$') {
            labels.push(server)
        }
    }

    new Chart(document.getElementById("submitPlot"), {
        type: "doughnut",
        data: {
            labels: labels,
            datasets: [{
                data: _dataByState(servers, 'submissions'),
                backgroundColor: serverColors
            }],
        },
        options: {
            title: {
                display: true,
                text: "Submissions"
            }
        }
    });

    new Chart(document.getElementById("executedPlot"), {
        type: "doughnut",
        data: {
            labels: labels,
            datasets: [{
                data: _dataByState(servers, 'transfers'),
                backgroundColor: serverColors
            }],
        },
        options: {
            title: {
                display: true,
                text: "Executed"
            }
        }
    });

    new Chart(document.getElementById("activePlot"), {
        type: "doughnut",
        data: {
            labels: labels,
            datasets: [{
                data: _dataByState(servers, 'active'),
                backgroundColor: serverColors
            }],
        },
        options: {
            title: {
                display: true,
                text: "Active"
            }
        }
    });

    new Chart(document.getElementById("stagingPlot"), {
        type: "doughnut",
        data: {
            labels: labels,
            datasets: [{
                data: _dataByState(servers, 'staging'),
                backgroundColor: serverColors
            }],
        },
        options: {
            title: {
                display: true,
                text: "Staging"
            }
        }
    });

    new Chart(document.getElementById("startedPlot"), {
        type: "doughnut",
        data: {
            labels: labels,
            datasets: [{
                data: _dataByState(servers, 'started'),
                backgroundColor: serverColors
            }],
        },
        options: {
            title: {
                display: true,
                text: "Started"
            }
        }
    });
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
            _generateServerPlots($scope.servers);
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    _generateServerPlots($scope.servers);
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

    var labels = []
    for (vo in vos) {
        if (vo[0] != '$') {
            labels.push(vo);
        }
    }

    new Chart(document.getElementById("activePlot"), {
        type: "doughnut",
        data: {
            labels: labels,
            datasets: [{
                data: _dataByState(vos, 'active'),
                backgroundColor: perVoColors
            }],
        },
        options: {
            title: {
                display: true,
                text: "Active"
            }
        }
    });

    new Chart(document.getElementById("submittedPlot"), {
        type: "doughnut",
        data: {
            labels: labels,
            datasets: [{
                data: _dataByState(vos, 'submitted'),
                backgroundColor: perVoColors
            }],
        },
        options: {
            title: {
                display: true,
                text: "Submitted"
            }
        }
    });
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
            _generatePerVoPlots($scope.vos);
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    _generatePerVoPlots($scope.vos);
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

