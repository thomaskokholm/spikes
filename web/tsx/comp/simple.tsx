/// <reference path="../typings/tsd.d.ts" />

import * as React from 'react';
//import * as ReactDOM from 'react-dom';

interface HelloWorldProps extends React.Props<any> {
    name: string;
}

export class HelloMessage extends React.Component<HelloWorldProps, {}> {
    render() {
        return <div>Hello {this.props.name}</div>;
    }
}

React.render(<HelloMessage name="Bo Lorentsen" />, document.body );