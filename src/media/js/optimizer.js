
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
    var emaData = [];
    var successData = [];
    var activeData = [];
    var filesizeBottom = [];
    var filesizeData = [];
    var filesizeTop = [];
    var labels = [];

    for (var i = 0; i < $scope.optimizer.evolution.items.length; ++i) {
        // avoid cluttering the plot
        labels.unshift("");
        throughputData.unshift($scope.optimizer.evolution.items[i].throughput/(1024*1024));
        emaData.unshift($scope.optimizer.evolution.items[i].ema/(1024*1024));
        successData.unshift($scope.optimizer.evolution.items[i].success);
        activeData.unshift($scope.optimizer.evolution.items[i].active);

        var filesize = $scope.optimizer.evolution.items[i].filesize_avg/(1024*1024);
        var stddev = $scope.optimizer.evolution.items[i].filesize_stddev/(1024*1024);

        filesizeBottom.unshift(filesize - stddev);
        filesizeData.unshift(filesize);
        filesizeTop.unshift(filesize + stddev);
    }

    // Throughput chart
    new Chart(document.getElementById("throughputPlot"), {
        type: "line",
        data: {
            labels: labels,
            datasets: [
                {
                    yAxisID: "active",
                    label: "Decision",
                    data: activeData,
                    backgroundColor: "rgba(0, 0, 0, 0)",
                    borderColor: "rgb(255, 0, 0)",
                },
                {
                    yAxisID: "throughput",
                    label: "EMA",
                    data: emaData,
                    backgroundColor: "rgba(0, 204, 0, 0.5)",
                },
                {
                    yAxisID: "throughput",
                    label: "Throughput",
                    data: throughputData,
                    backgroundColor: "rgba(102, 204, 255, 0.5)",
                },
            ]
        },
        options: {
            scales: {
                yAxes: [
                    {
                        id: "throughput",
                        type: "linear",
                        position: "left",
                        ticks: {
                            beginAtZero: true,
                            callback: function(value, index, values) {
                                return value + ' MB/s'
                            }
                        }
                    },
                    {
                        id: "active",
                        type: "linear",
                        position: "right",
                        ticks: {
                            beginAtZero: true,
                        }
                    },
                ]
            }
        }
    })

    // Success chart
    new Chart(document.getElementById("successPlot"), {
        type: "line",
        data: {
            labels: labels,
            datasets: [
                {
                    yAxisID: "active",
                    label: "Decision",
                    data: activeData,
                    backgroundColor: "rgba(0, 0, 0, 0)",
                    borderColor: "rgb(255, 0, 0)",
                },
                {
                    yAxisID: "success",
                    label: "Success rate",
                    data: successData,
                    backgroundColor: "rgba(0, 204, 0, 0.5)",
                }
            ]
        },
        options: {
            scales: {
                yAxes: [
                    {
                        id: "success",
                        type: "linear",
                        position: "left",
                        ticks: {
                            beginAtZero: true,
                            callback: function(value, index, values) {
                                return value + ' %'
                            }
                        }
                    },
                    {
                        id: "active",
                        type: "linear",
                        position: "right",
                        ticks: {
                            beginAtZero: true,
                        },
                    },
                ]
            }
        }
    })

    // Filesize plot
    new Chart(document.getElementById("filesizePlot"), {
        type: "line",
        data: {
            labels: labels,
            datasets: [
                {
                    yAxisID: "throughput",
                    label: "EMA",
                    data: emaData,
                    backgroundColor: "rgba(0, 0, 0, 0)",
                    borderColor: "rgb(0, 204, 0)",
                },
                {
                    yAxisID: "filesize",
                    label: "Min",
                    data: filesizeBottom,
                    backgroundColor: "rgba(255, 255, 255, 0)",
                    borderColor: "rgba(102, 204, 255, 0.5)",
                },
                {
                    yAxisID: "filesize",
                    label: "Average filesize",
                    data: filesizeData,
                    backgroundColor: "rgba(255, 255, 255, 0)",
                    borderColor: "rgb(0, 0, 255)",
                },
                {
                    yAxisID: "filesize",
                    label: "Max",
                    data: filesizeTop,
                    backgroundColor: "rgba(255, 255, 255, 0)",
                    borderColor: "rgba(102, 204, 255, 0.5)",
                },
            ]
        },
        options: {
            scales: {
                yAxes: [
                    {
                        id: "filesize",
                        type: "linear",
                        position: "right",
                        ticks: {
                            beginAtZero: true,
                            callback: function(value, index, values) {
                                return value + ' MB'
                            }
                        }
                    },
                    {
                        id: "throughput",
                        type: "linear",
                        position: "left",
                        ticks: {
                            beginAtZero: true,
                            callback: function(value, index, values) {
                                return value + ' MB/s'
                            }
                        },
                    },
                ]
            },
        }
    })
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
