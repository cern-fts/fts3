
function TransfersCtrl($location, $scope, transfers, Transfers, Unique)
{
	// Queue entries
	$scope.transfers = transfers;
	
	// Unique values
	$scope.unique = {
		sources: Unique('sources'),
		destinations: Unique('destinations'),
		vos: Unique('vos'),
		hostnames: Unique('hostnames')
	}

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.transfers.page;
    	$scope.transfers = Transfers.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
	
	// Set up filters
	$scope.filter = {
		vo:          validString($location.search().vo),
		source_se:   validString($location.search().source_se),
		dest_se:     validString($location.search().dest_se),
		source_surl: validString($location.search().source_surl),
		dest_surl:   validString($location.search().dest_surl),
		time_window: validString($location.search().time_window),
		state:       statesFromString($location.search().state),
		hostname:    validString($location.search().hostname),
	}
	
	$scope.applyFilters = function() {
		$location.search({
			page:         1,
			vo:           validString($scope.filter.vo),
			source_se:    $scope.filter.source_se,
			dest_se:      $scope.filter.dest_se,
			source_surl:  $scope.filter.source_surl,
			dest_surl:    $scope.filter.dest_surl,
			time_window:  $scope.filter.time_window,
			state:        joinStates($scope.filter.state),
			hostname:     validString($scope.filter.hostname),
		});
		$scope.filtersModal = false;
	}	
}


TransfersCtrl.resolve = {
	transfers: function($rootScope, $location, $q, Transfers) {
		loading($rootScope);
		
		var deferred = $q.defer();
	
		var page = $location.search().page;
		if (!page || page < 1)
			page = 1;
		
		Transfers.query($location.search(),
  			            genericSuccessMethod(deferred, $rootScope),
			            genericFailureMethod(deferred, $rootScope, $location));
		
		return deferred.promise;
	}
}
