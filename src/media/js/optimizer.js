
function getRowClass(diff)
{
    if (diff < 0) {
        return "optimizer-decrease";
    }
    else if (diff > 0) {
        return "optimizer-increase"
    }
    else {
        return "optimizer-steady";
    }
}

function OptimizerCtrl($rootScope, $location, $scope, optimizer, Optimizer)
{
    $scope.optimizer = optimizer;

    // Filter
    $scope.showFilterDialog = function() {
        document.getElementById('filterDialog').style.display = 'block';
    }

    $scope.cancelFilters = function() {
        document.getElementById('filterDialog').style.display = 'none';
    }

    $scope.filter = {
        source_se:   validString($location.$$search.source_se),
        dest_se:     validString($location.$$search.dest_se),
        time_window: parseInt($location.$$search.time_window)
    }

    $scope.applyFilters = function() {
        $location.search($scope.filter);
        document.getElementById('filterDialog').style.display = 'none';
    }

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        loading($rootScope);
        var filter = $location.$$search;
        filter.page = $scope.optimizer.page;
        Optimizer.query(filter, function(updatedOptimizer) {
            $scope.optimizer = updatedOptimizer;
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });
}


OptimizerCtrl.resolve = {
    optimizer: function ($rootScope, $location, $route, $q, Optimizer) {
        loading($rootScope);

        var deferred = $q.defer();

        Optimizer.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}


function OptimizerDetailedCtrl($rootScope, $location, $scope, optimizer, OptimizerDetailed)
{
    $scope.optimizer = optimizer;
    $scope.getRowClass = getRowClass;

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        loading($rootScope);
        var filter = $location.$$search;
        filter.page = $scope.optimizer.page;
        OptimizerDetailed.query(filter, function(updatedOptimizer) {
            $scope.optimizer = updatedOptimizer;
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    // Page
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

    // Set up filters
    $scope.filter = {
        source:      validString($location.$$search.source),
        destination: validString($location.$$search.destination)
    }

    var throughputData = [];
    var successData = [];
    for (var i = 0; i < $scope.optimizer.evolution.items.length; ++i) {
        var label = $scope.optimizer.evolution.items[i].datetime;
        throughputData.unshift({
            x: label,
            y: [
                $scope.optimizer.evolution.items[i].active,
                $scope.optimizer.evolution.items[i].throughput
            ]
        });
        successData.unshift({
            x: label,
            y: [
                $scope.optimizer.evolution.items[i].active,
                $scope.optimizer.evolution.items[i].success,
            ]
        });
    }

    $scope.plots = {
        throughput: {
            series: ['Active', 'Weighted throughput'],
            data: throughputData,
            config: {
                title: 'Throughput evolution',
                colors: ["#0000FF", "#00FF00"],
                doubleYAxis: true,
                labels: ['Active', 'Weighted throughput']
            }
        },
        success: {
            series: ['Active', 'Success'],
            data: successData,
            config: {
                title: 'Success rate evolution',
                colors: ["#0000FF", "#00FF00"],
                doubleYAxis: true,
                labels: ['Active', 'Success']
            }
        }
    };
}


OptimizerDetailedCtrl.resolve = {
    optimizer: function ($rootScope, $location, $route, $q, OptimizerDetailed) {
        loading($rootScope);

        var deferred = $q.defer();

        OptimizerDetailed.query($location.$$search,
            genericSuccessMethod(deferred, $rootScope),
            genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}


function OptimizerStreamsCtrl($rootScope, $location, $scope, streams, OptimizerStreams)
{
    $scope.streams = streams;

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        var filter = $location.$$search;
        filter.page = $scope.optimizer.page;
        OptimizerStreams.query(filter, function(updatedStreams) {
            $scope.streams = updatedStreams;
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    // Page
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };
}


OptimizerStreamsCtrl.resolve = {
    streams: function($rootScope, $location, $route, $q, OptimizerStreams) {
        loading($rootScope);

        var deferred = $q.defer();

        OptimizerStreams.query($location.$$search,
            genericSuccessMethod(deferred, $rootScope),
            genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}
