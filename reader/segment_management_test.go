package reader

import (
	"testing"
)

func TestSegmentManagement_SegmentsManagement(t *testing.T) {
	// Construct node, collection, partition and segment
	node := NewQueryNode(0, 0)
	var collection = node.NewCollection("collection0", "fake schema")
	var partition = collection.NewPartition("partition0")
	var segment = partition.NewSegment(0)
	node.SegmentsMap[0] = segment

	// TODO: fix segment management
	node.SegmentsManagement()
}

func TestSegmentManagement_SegmentService(t *testing.T) {
	// Construct node, collection, partition and segment
	node := NewQueryNode(0, 0)
	var collection = node.NewCollection("collection0", "fake schema")
	var partition = collection.NewPartition("partition0")
	var segment = partition.NewSegment(0)
	node.SegmentsMap[0] = segment

	// TODO: fix segment service
	node.SegmentService()
}