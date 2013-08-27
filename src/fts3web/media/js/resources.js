angular.module('ftsmon.resources', ['ngResource'])
.factory('Job', function($resource) {
	return $resource('jobs/:jobId', {}, {
		query: {method: 'GET',
			    isArray: false},
	})
})
.factory('Unique', function($resource) {
	return $resource('unique/', {}, {
		all: {method: 'GET', isArray: false}
	})
})
.factory('QueuePairs', function($resource) {
	return $resource('queue', {}, {
		query: {method: 'GET', isArray: false}
	})
})
.factory('Queue', function($resource) {
	return $resource('queue/detailed', {}, {
		query: {method: 'GET', isArray: false}
	})
})
.factory('Staging', function($resource) {
	return $resource('staging', {}, {
		query: {method: 'GET', isArray: false}
	})
});
