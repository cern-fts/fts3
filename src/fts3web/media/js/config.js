
function ConfigAuditCtrl($location, $scope, config, ConfigAudit)
{
	$scope.config = config;

	// Filter
	$scope.filter = {
		action:   undefinedAsEmpty($location.search().action),
		user:     undefinedAsEmpty($location.search().user),
		contains: undefinedAsEmpty($location.search().contains)
	};
	
	$scope.applyFilters = function() {
		$location.search({
			page:        1,
			action:   $scope.filter.action,
			user:     $scope.filter.user,
			contains: $scope.filter.contains
		});
	}
	
	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.config.page;
    	$scope.config = Configuration.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}

ConfigAuditCtrl.resolve = {
	config: function ($rootScope, $location, $route, $q, ConfigAudit) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	ConfigAudit.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}


function ConfigStatusCtrl($location, $scope, server, links)
{
	$scope.server = server;
	$scope.links = links;
	
	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};
}

ConfigStatusCtrl.resolve = {
	server: function ($rootScope, $location, $route, $q, ConfigServer) {
		loading($rootScope);
		
		var deferred = $q.defer();
		
		ConfigServer.all(function(data) {
			deferred.resolve(data);
			stopLoading($rootScope);
		});
		
		return deferred.promise;
	},
	
	links: function($rootScope, $location, $route, $q, ConfigLinks) {
		loading($rootScope);
		
		var deferred = $q.defer();
		
		ConfigLinks.query($location.search(), function(data) {
			deferred.resolve(data);
			stopLoading($rootScope);
		});
		
		return deferred.promise;
	}
}
