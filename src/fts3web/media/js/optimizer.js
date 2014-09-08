
function OptimizerCtrl($location, $scope, optimizer, Optimizer)
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
		source_se:   validString($location.search().source_se),
		dest_se:     validString($location.search().dest_se),
		time_window: parseInt($location.search().time_window)
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
		var filter = $location.search();
		filter.page = $scope.optimizer.page;
    	$scope.optimizer = Optimizer.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


OptimizerCtrl.resolve = {
	optimizer: function ($rootScope, $location, $route, $q, Optimizer) {
    	loading($rootScope);

    	var deferred = $q.defer();

    	Optimizer.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

    	return deferred.promise;
	}
}


function OptimizerDetailedCtrl($location, $scope, optimizer, OptimizerDetailed)
{
	$scope.optimizer = optimizer;

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.optimizer.page;
    	$scope.optimizer = OptimizerDetailed.query(filter);
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
        source:      validString($location.search().source),
        destination: validString($location.search().destination)
	}
}


OptimizerDetailedCtrl.resolve = {
	optimizer: function ($rootScope, $location, $route, $q, OptimizerDetailed) {
    	loading($rootScope);

    	var deferred = $q.defer();

    	OptimizerDetailed.query($location.search(),
            genericSuccessMethod(deferred, $rootScope),
            genericFailureMethod(deferred, $rootScope, $location));

    	return deferred.promise;
	}
}


function OptimizerStreamsCtrl($location, $scope, streams, OptimizerStreams)
{
    $scope.streams = streams;

    // Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.optimizer.page;
    	$scope.streams = OptimizerStreams.query(filter);
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

        OptimizerStreams.query($location.search(),
            genericSuccessMethod(deferred, $rootScope),
			genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}
