
function StatsOverviewCtrl($location, $scope, stats, Statistics)
{
	$scope.stats = stats;
	
	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.stats = Statistics.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsOverviewCtrl.resolve = {
	stats: function($rootScope, $location, $q, Statistics) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	Statistics.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}


function StatsServersCtrl($location, $scope, servers, Servers)
{
	$scope.servers = servers;
	
	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.servers = Servers.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsServersCtrl.resolve = {
	servers: function($rootScope, $location, $q, Servers) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	Servers.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}


function StatsVosCtrl($location, $scope, vos, StatsVO, Unique)
{
	$scope.vos = vos;
	
	// Filter
	$scope.unique = Unique.all();
	
	$scope.filter = {
		'source_se': undefinedAsEmpty($location.search().source_se),
		'dest_se':   undefinedAsEmpty($location.search().dest_se),
	}
	
	$scope.applyFilters = function() {
		$location.search($scope.filter);
	}

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.vos = StatsVO.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsVosCtrl.resolve = {
	vos: function($rootScope, $location, $q, StatsVO) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	StatsVO.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}


function StatsProfilingCtrl($location, $scope, profile, Profile)
{
	$scope.profile = profile;

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
    	$scope.profile = Profile.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StatsProfilingCtrl.resolve = {
	profile: function($rootScope, $location, $q, Profile) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	Profile.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}
