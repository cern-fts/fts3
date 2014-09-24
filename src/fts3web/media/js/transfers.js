
function TransfersCtrl($location, $scope, transfers, Transfers, Unique)
{
	$scope.unique = {
		activities: Unique('activities'),
		hostnames: Unique('hostnames')
	}
	// Queue entries
	$scope.transfers = transfers;

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.$$search;
		filter.page = $scope.transfers.page;
    	$scope.transfers = Transfers.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});

	// Set up filters
	$scope.filter = {
		vo:          validString($location.$$search.vo),
		source_se:   validString($location.$$search.source_se),
		dest_se:     validString($location.$$search.dest_se),
		source_surl: validString($location.$$search.source_surl),
		dest_surl:   validString($location.$$search.dest_surl),
		time_window: parseInt(validString($location.$$search.time_window)),
		state:       statesFromString($location.$$search.state),
		activity:    validString($location.$$search.activity),
		hostname:    validString($location.$$search.hostname),
	}

    $scope.showFilterDialog = function() {
    	document.getElementById('filterDialog').style.display = 'block';
    }

    $scope.cancelFilters = function() {
    	document.getElementById('filterDialog').style.display = 'none';
    }

	$scope.applyFilters = function() {
		$location.search({
			page:         1,
			source_surl:  $scope.filter.source_surl,
			dest_surl:    $scope.filter.dest_surl,
			time_window:  $scope.filter.time_window,
			state:        joinStates($scope.filter.state),
			activity:     validString($scope.filter.activity),
			hostname:     validString($scope.filter.hostname),
		});
		document.getElementById('filterDialog').style.display = 'none';
	}
}


TransfersCtrl.resolve = {
	transfers: function($rootScope, $location, $q, Transfers) {
		loading($rootScope);

		var deferred = $q.defer();

		var page = $location.$$search.page;
		if (!page || page < 1)
			page = 1;

		Transfers.query($location.$$search,
  			            genericSuccessMethod(deferred, $rootScope),
			            genericFailureMethod(deferred, $rootScope, $location));

		return deferred.promise;
	}
}
