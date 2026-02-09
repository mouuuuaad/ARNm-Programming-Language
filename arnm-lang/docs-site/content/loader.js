// Documentation Loader - Combines all content sections
// This file loads all modular content files and combines them

window.documentationContent = [];

// Content will be added by each section file
window.addDocSection = function (section) {
    window.documentationContent.push(section);
};

window.addDocSections = function (sections) {
    sections.forEach(function (section) {
        window.documentationContent.push(section);
    });
};
