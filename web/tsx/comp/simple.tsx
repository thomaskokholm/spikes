/// <reference path="../typings/tsd.d.ts" />

import * as React from 'react';
import * as ReactDOM from 'react-dom';

interface CounterProps extends React.Props<any> {
    name: string;
}

interface CounterState {
    count: number;
}

export class Counter extends React.Component<CounterProps, CounterState> {
    state = { count: 0 };

    constructor(props: CounterProps) {
        super( props );

        setInterval(() => {
            this.setState({count: this.state.count + 1});
        }, 1000);
    }

    render() {
        return <div>{this.props.name} : {this.state.count}</div>;
    }
}

ReactDOM.render(<Counter name="Counter" />, document.getElementById( "test_app"));