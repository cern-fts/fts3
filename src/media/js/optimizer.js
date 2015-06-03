
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
            series: ['Active', 'Throughput'],
            data: throughputData,
            config: {
                title: 'Throughput evolution',
                colors: ["#0000FF", "#00FF00"],
                doubleYAxis: true,
                labels: ['Active', 'Throughput']
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

// From
// http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
function hsvToRgb(h, s, v){
    var r, g, b;

    var i = Math.floor(h * 6);
    var f = h * 6 - i;
    var p = v * (1 - s);
    var q = v * (1 - f * s);
    var t = v * (1 - (1 - f) * s);

    switch(i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    return [r * 255, g * 255, b * 255];
}


function quantileColor(stream)
{
    var n_quantiles = stream.quantiles;
    var quantile = stream.quantile;

    var h_start = 0;
    var h_end   = 0.333;

    var h = h_start + ((h_end - h_start) * ((quantile - 1) / n_quantiles));

    var rgb = hsvToRgb(h, 1, 1);
    var color = 'rgba(' + Math.ceil(rgb[0]) + ',' + Math.ceil(rgb[1]) + ',' + Math.ceil(rgb[2]) + ', 0.8)';

    return {'border-left': 'solid 10px ' + color};
}


function OptimizerStreamsCtrl($rootScope, $location, $scope, streams, OptimizerStreams)
{
    $scope.streams = streams;
    $scope.quantileColor = quantileColor;

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
