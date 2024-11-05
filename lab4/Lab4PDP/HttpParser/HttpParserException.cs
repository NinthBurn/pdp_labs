using System;

namespace Lab4PDP.HttpParser
{
    public class HttpParserException : Exception
    {
        public HttpParserException(string message, string step, string component)
            : base($"{message} Method: {step}() Data: {component}") { }
    }
}
