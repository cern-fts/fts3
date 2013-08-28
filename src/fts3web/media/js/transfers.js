
function TransfersCtrl($location, $scope, transfers, Transfers, Unique)
{
	// Queue entries
	$scope.transfers = transfers;
	
	// Unique values
	$scope.unique = Unique.all();
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.transfers.page;
	$scope.pageCount = $scope.transfers.pageCount;

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
		vo:          undefinedAsEmpty($location.search().vo),
		source_se:   undefinedAsEmpty($location.search().source_se),
		dest_se:     undefinedAsEmpty($location.search().dest_se),
		time_window: undefinedAsEmpty($location.search().time_window),
		state:       statesFromString($location.search().state)
	}
	
	$scope.applyFilters = function() {
		$location.search({
			page:         1,
			vo:           validVo($scope.filter.vo),
			source_se:    $scope.filter.source_se,
			dest_se:      $scope.filter.dest_se,
			time_window:  $scope.filter.time_window,
			state:        joinStates($scope.filter.state)
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
		
		Transfers.query($location.search(), function(data) {
			deferred.resolve(data);
			stopLoading($rootScope);
		});
		
		return deferred.promise;
	}
}
