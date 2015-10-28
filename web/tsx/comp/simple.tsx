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
    constructor(props: CounterProps) {
        super( props );

        this.state = { count: 0 };

        console.log(this.state);

        setInterval(() => {
            this.setState({count: this.state.count + 1});
        }, 1000);
    }

    getInitialState() : CounterState {
        return { count: 0 };
    }

    render() {
        return <div>{this.props.name} : {this.state.count}</div>;
    }
}

ReactDOM.render(<Counter name="Counter" />, document.body );