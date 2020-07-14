
loadScript('qrc:///qtwebchannel/qwebchannel.js').then(() => {
    new QWebChannel(qt.webChannelTransport, (channel) => {
        const sourceTextObject = channel.objects.sourceTextObject;
        const viewObject = channel.objects.viewObject;
        const origScrollToFirstChange = scrollToFirstChange;

        window.onscroll = function() {
            viewObject.setScrollPosition(window.scrollX, window.scrollY);
        };

        viewObject.requestSetScrollPosition.connect((x, y) => {
            window.scrollTo(x, y);
        });

        sourceTextObject.urlChanged.connect((url) => {
            // Prevent scroll/highlight of first change when loading a new document
            // This is useless because
            // - first change will always be first block
            // - we scroll to previous scroll position anyway in this case (requestSetScrollPosition)
            scrollToFirstChange = () => {
                scrollToFirstChange = origScrollToFirstChange;
            };
        });

        // Tell C++ whenever rendering of a new content finishes
        document.addEventListener('pmpmRenderingDone', (_) => {
            viewObject.emitRenderingDone();
        });

        // Tell C++ code that we are ready to take content
        getWebsocket().then(() => {
            viewObject.emitInitDone();
        });
    });
});
