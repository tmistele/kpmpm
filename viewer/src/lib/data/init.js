
loadScript('qrc:///qtwebchannel/qwebchannel.js').then(() => {
    new QWebChannel(qt.webChannelTransport, (channel) => {
        const sourceTextObject = channel.objects.sourceTextObject;
        const viewObject = channel.objects.viewObject;

        /**
         * TODO:
         * - cpp: Save/restore scroll position per document, not per kpart instance
         *   - this would be best to do in kate preview addon instead of kpart. --> make this a kkatepmpm plugin instead of general preview plugin?
         * - js here: don't directly scroll? give a hint to pmpm.js to do scrollToFirstChange only when document edit (not when document change)
         *   Can we detect this using sourceTextObject.textChanged.connect??
         */
        // For now this doesn't do much, so just comment it out
        /*
        window.onscroll = function() {

            viewObject.setScrollPosition(window.scrollX, window.scrollY);
        };
        viewObject.requestSetScrollPosition.connect((x, y) => {
            window.scrollTo(x, y);
        });
        */

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
