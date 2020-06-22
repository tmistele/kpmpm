
loadScript('qrc:///qtwebchannel/qwebchannel.js').then(() => {
    new QWebChannel(qt.webChannelTransport, (channel) => {
        const sourceTextObject = channel.objects.sourceTextObject;
        const viewObject = channel.objects.viewObject;

        window.onscroll = function() {
            viewObject.setScrollPosition(window.scrollX, window.scrollY);
        };

        viewObject.requestSetScrollPosition.connect((x, y) => {
            // TODO: on document switch we can try to prevent the unnecessary 'scrollToFirstChange' in pmpm.js
            window.scrollTo(x, y);
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
