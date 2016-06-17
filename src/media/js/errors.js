

function ErrorsCtrl($rootScope, $location, $scope, pairs, Errors)
{
    // Errors
    $scope.pairs = pairs;

    // Filter
    $scope.showFilterDialog = function() {
        document.getElementById('filterDialog').style.display = 'block';
    }

    $scope.cancelFilters = function() {
        document.getElementById('filterDialog').style.display = 'none';
    }

    $scope.applyFilters = function() {
        var filter = $scope.filter;
        filter['source_se'] = validString($location.$$search.source_se);
        filter['dest_se'] = validString($location.$$search.dest_se);
        $location.search(filter);
        document.getElementById('filterDialog').style.display = 'none';
    }

    $scope.filter = {
        source_se: validString($location.$$search.source_se),
        dest_se:   validString($location.$$search.dest_se),
        reason:    validString($location.$$search.reason)
    }

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        loading($rootScope);
        var filter = $location.$$search;
        filter.page = $scope.pairs.page;
        Errors.query(filter, function(updatedErrors) {
            $scope.pairs = updatedErrors;
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });
}


ErrorsCtrl.redirectTo = function(routeParams, path, search)
{
    if (search.source_se && search.dest_se)
        return '/errors/list?source_se=' + search.source_se + '&dest_se=' + search.dest_se + '&time_window=' + search.time_window;
    else
        return '/errors/pairs?source_se=' + validString(search.source_se) + '&dest_se=' + validString(search.dest_se) + '&time_window=' + validString(search.time_window);
}


ErrorsCtrl.resolve = {
    pairs: function($rootScope, $q, $location, Errors) {
        loading($rootScope);

        var deferred = $q.defer();

        var page = $location.$$search.page;
        if (!page || page < 1)
            page = 1;

        Errors.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

function _countPerClassification(classification)
{
    var result = [];
    for (phase in classification) {
        if (phase[0] != '$') {
            var value = classification[phase];
            result.push({x: phase, y: [value]});
        }
    }
    return result;
}

function ErrorsForPairCtrl($location, $scope, errors, ErrorsForPair)
{
    $scope.reason    = $location.$$search.reason;
    $scope.errors    = errors
    $scope.source_se = $location.$$search.source_se;
    $scope.dest_se   = $location.$$search.dest_se;

    // Filter
    $scope.showFilterDialog = function() {
        document.getElementById('filterDialog').style.display = 'block';
    }

    $scope.cancelFilters = function() {
        document.getElementById('filterDialog').style.display = 'none';
    }

    $scope.applyFilters = function() {
        $location.search($scope.filter);
        $scope.filtersModal = false;
    }

    $scope.filter = {
        source_se: validString($location.$$search.source_se),
        dest_se:   validString($location.$$search.dest_se),
        reason:    validString($location.$$search.reason)
    }

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

    // Generate plot
    var labels = []
    var count = []
    for (phase in $scope.errors.classification) {
        if (phase[0] != '$') {
            labels.push(phase);
            count.push($scope.errors.classification[phase])
        }
    }

    var phasePlot = new Chart(document.getElementById("phasePlot"), {
        type: "pie",
        data: {
            labels: labels,
            datasets: [{
                data: count,
                backgroundColor: ['#366DD8', '#D836BE', '#D8A136', '#36D850'],
            }],
        },

    });
    /*
    $scope.plots = {
        phase: {
            data: _countPerClassification($scope.errors.classification),
            config: {
                title: 'Error phase',
                colors: ['#366DD8', '#D836BE', '#D8A136', '#36D850'],
                labels: true,
                legend: {
                    display: true,
                    position: 'right'
                }
            }
        }
    };
    */
}



ErrorsForPairCtrl.resolve = {
    errors: function($rootScope, $q, $location, ErrorsForPair) {
        loading($rootScope);

        var deferred = $q.defer();

        var page = $location.$$search.page;
        if (!page || page < 1)
            page = 1;

        ErrorsForPair.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              function () {
                  stopLoading($rootScope);
                  $location.path('/errors/pairs');
              });

        return deferred.promise;
    }
}
