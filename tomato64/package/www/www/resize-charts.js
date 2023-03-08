function resize_graph() {
	var targetNode = top.document.getElementById('content');
	if (targetNode && targetNode.tagName === 'TD') {
		targetNode.classList.add('dynamic');
	}

	var graph = top.document.getElementById('graph');
	var dest = top.document.getElementsByTagName('embed')[0];

	const observer = new ResizeObserver(entries => {
		vWidth = graph.clientWidth - 10;
		vHeight = parseInt(vWidth * 0.35);

		dest.style.width = vWidth + 'px';
		dest.style.height = vHeight + 'px';

		top.initData();
	});
	observer.observe(graph);
}
